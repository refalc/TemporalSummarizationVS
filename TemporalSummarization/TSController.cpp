#include "TSController.h"
#include "TSDataExtractor.h"
#include "TSQueryConstructor.h"
#include "TSDocumentExtractor.h"
#include "TSSolver.h"
#include "TSPrimitives.h"
#include <fstream>

TSController::TSController() :
	m_iTemporalSummarySize(0)
{
	m_spDataExtractor.reset(new TSDataExtractor);
	m_spQueryConstructor.reset(new TSQueryConstructor);
	m_spDocExtractor.reset(new TSDocumentExtractor);
	m_spSolver.reset(new TSSolver);
}


TSController::~TSController()
{}

bool TSController::InitParameters(const Params &params, const std::string &answer_path, const std::string &w2v_path, int summary_size)
{
	if( !m_spQueryConstructor->InitDataExtractor(m_spDataExtractor.get()) )
		return false;

	if( !m_spDocExtractor->InitDataExtractor(m_spDataExtractor.get()) )
		return false;

	if( !m_spSolver->InitDataExtractor(m_spDataExtractor.get()) )
		return false;

	m_Params = params;
	m_AnswerPath = answer_path;
	m_W2VPath = w2v_path;
	m_iTemporalSummarySize = summary_size;

	m_spModel.reset(new Word2Vec);
	if( m_Params.m_DIW2VEnable || m_Params.m_SlvW2VEnable ) {
		if( !m_spModel->Load(m_W2VPath) )
			return false;

		if( !m_spDocExtractor->InitW2VModel(m_spModel.get()) )
			return false;

		if( !m_spSolver->InitW2VModel(m_spModel.get()) )
			return false;
	}
	/*
	m_iLemmsFor1L
	m_iTermsFor1L
	m_iDocCountForQEProcess
	m_fSoftOrForQEProcess
	m_fMinDocRankForQEProcess
	m_iTopLemmsForQEProcess
	m_iTopTerminsForQEProcess
	m_iResultLemmaSize
	m_iResultTerminsSize
	*/
	auto query_constructor_params = { (float)m_Params.m_PKeepL, (float)m_Params.m_PKeepT, (float)m_Params.m_QEDocCount,
		                              (float)m_Params.m_QESoftOr, (float)m_Params.m_QEMinDocRank, (float)m_Params.m_QETopLemms,
		                              (float)m_Params.m_QETopTermins, (float)m_Params.m_QEQuerrySize,
		        /*todo add new param*/(float)m_Params.m_PKeepT, (float)m_Params.m_QEDEInitQuerrySize };
	if( !m_spQueryConstructor->InitParameters(query_constructor_params) )
		return false;

	/*
	m_iDocCount
	m_fSoftOr
	m_fMinDocRank
	*/
	auto doc_extractor_params = { (float)m_Params.m_PDocCount, (float)m_Params.m_PSoftOr, /*todo add new param or delete*/0.f,
		                          (float)m_Params.m_DIMinLinkScore, (float)m_Params.m_DIPowerMethodDFactor, (float)m_Params.m_DIDocBoundary,
		                          (float)m_Params.m_DocImportance, (float)m_Params.m_PTemporalMode, (float)m_Params.m_DIW2VEnable };
	if( !m_spDocExtractor->InitParameters(doc_extractor_params) )
		return false;

	/*
	m_iMaxDailyAnswerSize
	m_fLambda
	m_fSimThreshold
	m_fMinMMR
	*/
	auto solver_params = { (float)m_Params.m_TempMaxDailyAnswerSize, (float)m_Params.m_PLambda, /*todo add new param or delete*/0.8f,
		                   (float)m_Params.m_PMinMMR, (float)m_Params.m_DocImportance, (float)m_Params.m_DIAlpha, (float)m_Params.m_SlvW2VEnable };
	if( !m_spSolver->InitParameters(solver_params) )
		return false;

	return true;
}

bool TSController::RunQueries(const std::vector<std::string> &queries) const
{
	auto probe = CProfiler::CProfilerProbe("run_queries");
	CLogger::Instance()->WriteToLog("INFO : Start run queries, size = " + std::to_string(queries.size()));
	if( !CleanAnswerFile() )
		return false;

#pragma omp parallel
	{
#pragma omp  for
	for( int i = 0; i < queries.size(); i++ )
		if( !RunQuery(queries[i]) ) {
			// save empty results
			CLogger::Instance()->WriteToLog("ERROR : save empty result, doc_id = " + queries[i]);

			std::vector<std::pair<float, TSSentenceConstPtr>> temporal_summary;
			TSTimeLineQueries ts_queries;

			bool save_result = false;
#pragma omp critical (save_temporal_summary)
			{
				save_result = SaveTemporalSummaryInFile(temporal_summary, ts_queries, queries[i]);
			}
			if( !save_result ) {
				CLogger::Instance()->WriteToLog("ERROR : error while save empty temporal summary, doc_id = " + queries[i]);
			}
		}
	}
	return true;
}

bool TSController::RunQuery(const std::string &doc_id) const
{
	CLogger::Instance()->WriteToLog("INFO : Start run query, id = " + doc_id);
	if( !m_spDataExtractor || !m_spQueryConstructor || !m_spDocExtractor || !m_spSolver )
		return false;

	TSQuery query_first_level, query_second_level, query_third_level, query;
	if( !m_spQueryConstructor->QueryConstructionProcess(doc_id, query_first_level) ) {
		CLogger::Instance()->WriteToLog("ERROR : error while query construction process");
		return false;
	}

	if( m_Params.m_PQuerryEx ) {
		if( !m_spQueryConstructor->FullQueryExtensionProcess(query_first_level, m_Params.m_QEDoubleExtension, query) ) {
			CLogger::Instance()->WriteToLog("ERROR : error while query extension process. Query will");
			return false;
		}
	} else
		query = query_first_level;

	TSTimeLineCollections collection;
	if( !m_spDocExtractor->ConstructTimeLineCollections(query, collection) ) {
		CLogger::Instance()->WriteToLog("ERROR : error while construct time collection");
		return false;
	}

	TSTimeLineQueries queries;
	ConstructTimeLinesQueries(std::move(query), doc_id, collection, queries);

	std::vector<std::pair<float, TSSentenceConstPtr>> temporal_summary;
	CLogger::Instance()->WriteToLog("INFO : start solve collection size = " + std::to_string(collection.size()) + " queries size = " + std::to_string((int)std::distance(queries.begin(), queries.end())));
	if( !m_spSolver->GetTemporalSummary(collection, queries, m_iTemporalSummarySize, temporal_summary) ) {
		CLogger::Instance()->WriteToLog("ERROR : error while construct temporal summary");
		return false;
	}

	bool save_result = false;
#pragma omp critical (save_temporal_summary)
	{
		save_result = SaveTemporalSummaryInFile(temporal_summary, queries, doc_id);
	}

	if( !save_result ) {
		CLogger::Instance()->WriteToLog("ERROR : error while save temporal summary, doc_id = " + doc_id);
		return false;
	}

	return true;
}

bool TSController::SaveTemporalSummaryInFile(const std::vector<std::pair<float, TSSentenceConstPtr>> &temporal_summary, const TSTimeLineQueries &queries, const std::string &init_doc_id) const
{
	std::fstream pFile;
	pFile.open(m_AnswerPath, std::fstream::app);
	if( !pFile.is_open() )
		return false;

	pFile << "<story id=" << m_iStoryIDCounter++ << " init_doc_id=" << init_doc_id << ">" << std::endl;
	pFile << "<queries>" << std::endl;
	for( const auto &query_pair : queries ) {
		pFile << "<query int_date=" << query_pair.first << ">" << std::endl;

		TSIndexConstPtr lemma_index_ptr, termin_index_ptr;
		if( !query_pair.second.GetIndex(SDataType::LEMMA, lemma_index_ptr) )
			return false;
		pFile << "<lemmas>" << std::endl;
		pFile << lemma_index_ptr->GetString() << std::endl;
		pFile << "</lemmas>" << std::endl;

		if( query_pair.second.GetIndex(SDataType::TERMIN, termin_index_ptr) ) {
			pFile << "<termins>" << std::endl;
			pFile << termin_index_ptr->GetString() << std::endl;
			pFile << "</termins>" << std::endl;
		}
		pFile << "</query>" << std::endl;
	}
	pFile << "</queries>" << std::endl;

	pFile << "<summary>" << std::endl;
	for( int i = 0; i < temporal_summary.size(); i++ ) {
		pFile << "<sentence id=" << i + 1 << " mmr=" << temporal_summary[i].first << ">" << std::endl;

		TSDocumentConstPtr pdoc = temporal_summary[i].second->GetDocPtr();
		pFile << "<metadata";
		std::string meta_data;
		if( !pdoc->GetMetaData(SMetaDataType::DATE, meta_data) )
			return false;
		pFile << " date=" << meta_data;
		if( !pdoc->GetMetaData(SMetaDataType::SITE, meta_data) )
			return false;
		pFile << " site=" << meta_data;
		if( !pdoc->GetMetaData(SMetaDataType::TITLE, meta_data) )
			return false;
		pFile << " title=" << meta_data;
		pFile << " doc_id=" << pdoc->GetDocID();
		pFile << " sent_num=" << temporal_summary[i].second->GetSentenseNumber();
		pFile << "\\>" << std::endl;

		TSIndexConstPtr word_index_ptr;
		if( !temporal_summary[i].second->GetIndex(SDataType::WORD, word_index_ptr) )
			return false;
		pFile << word_index_ptr->GetOrderedString() << std::endl;
		
		pFile << "</sentence>" << std::endl;
	}
	pFile << "</summary>" << std::endl;
	pFile << "</story>" << std::endl;
	pFile.close();

	return true;
}

bool TSController::CleanAnswerFile() const
{
	m_iStoryIDCounter = 0;

	std::fstream pFile;
	pFile.open(m_AnswerPath, std::fstream::out);
	if( !pFile.is_open() )
		return false;

	pFile.close();
	return true;
}

bool TSController::ConstructTimeLinesQueries(TSQuery &&init_query, const std::string &init_doc_id, const TSTimeLineCollections &collections, TSTimeLineQueries &queries) const
{
	int query_int_date = GetQueryDate(init_doc_id, collections);

	if( m_Params.m_SlvW2VEnable ) {
		TSIndexConstPtr p_index;
		if( !init_query.GetIndex(SDataType::LEMMA, p_index) )
			return false;
		p_index->ConstructIndexEmbedding(m_spModel.get());
	}
	if( !queries.AddQuery(query_int_date, std::move(init_query)) )
		return false;

	if( m_Params.m_DocImportance ) {
		for( const auto &doc_id : collections.GetTopDocs() ) {
			TSQuery query_first_level, query_second_level, query_third_level, query;
			if( !m_spQueryConstructor->QueryConstructionProcess(doc_id, query_first_level) ) {
				CLogger::Instance()->WriteToLog("ERROR : error while query construction process");
				return false;
			}

			if( m_Params.m_PQuerryEx ) {
				if( !m_spQueryConstructor->FullQueryExtensionProcess(query_first_level, m_Params.m_QEDoubleExtension, query) ) {
					CLogger::Instance()->WriteToLog("ERROR : error while query extension process. Query will");
					return false;
				}
			}
			else
				query = query_first_level;

			if( m_Params.m_SlvW2VEnable ) {
				TSIndexConstPtr p_index;
				if( !query.GetIndex(SDataType::LEMMA, p_index) )
					return false;
				p_index->ConstructIndexEmbedding(m_spModel.get());
			}

			int query_int_date = GetQueryDate(doc_id, collections);
			if( !queries.AddQuery(query_int_date, std::move(query)) )
				return false;
		}
	} 

	return true;
}

int TSController::GetQueryDate(const std::string &init_doc_id, const TSTimeLineCollections &collections) const
{
	int query_int_date = 0;
	for( const auto &day_collection : collections ) {
		for( const auto &doc : day_collection.second ) {
			if( doc.first == init_doc_id ) {
				std::string date;
				if( doc.second.GetMetaData(SMetaDataType::INT_DATE, date) )
					query_int_date = std::stoi(date);
			}
		}
	}

	if( query_int_date == 0 ) {
		TSDocCollection temp_collection;
		TSDocumentPtr doc_ptr = temp_collection.AllocateDocument();
		if( m_spDataExtractor->GetDocument(init_doc_id, doc_ptr) == ReturnCode::TS_NO_ERROR ) {
			std::string date;
			if( doc_ptr->GetMetaData(SMetaDataType::INT_DATE, date) )
				query_int_date = std::stoi(date);
		}
		temp_collection.CommitAllocatedDocument();
	}

	return query_int_date;
}