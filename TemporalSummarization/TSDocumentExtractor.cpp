#include "TSDocumentExtractor.h"
#include "TSDataExtractor.h"
#include <winsock.h>
#include <iostream>

TSDocumentExtractor::TSDocumentExtractor() :
	m_iDocTailSize(4),
	m_iDocHeadSize(3)
{
}


TSDocumentExtractor::~TSDocumentExtractor()
{
}

bool TSDocumentExtractor::InitParameters(const std::initializer_list<float> &params)
{
	constexpr int parameters_size = 8;
	if( params.size() < parameters_size )
		return false;

	int doc_count = (int)params.begin()[0];
	float soft_or = params.begin()[1],
		  min_doc_rank = params.begin()[2],
		  min_link_score = params.begin()[3],
		  power_d_factor = params.begin()[4],
		  importance_boundary = params.begin()[5];
	bool is_doc_importance = (bool)params.begin()[6],
		 is_temporal = (bool)params.begin()[7],
		 is_w2v = (bool)params.begin()[8];

	if( doc_count < 0 || soft_or < 0.0f || soft_or > 1.f ||
		min_doc_rank < 0.0f || min_doc_rank > 1.0f ||
		min_link_score < 0.0f || min_link_score > 1.0f ||
		power_d_factor < 0.0f || power_d_factor > 1.0f ||
		importance_boundary < 0.0f || importance_boundary > 1.0f)
		return false;

	m_iDocCount = doc_count;
	m_fSoftOr = soft_or;
	m_fMinDocRank = min_doc_rank;
	m_fMinLinkScore = min_link_score;
	m_fPowerDFactor = power_d_factor;
	m_fDocumentImportanceBoundary = importance_boundary;
	m_bDocImportance = is_doc_importance;
	m_bTemporalMode = is_temporal;
	m_bIsW2V = is_w2v;

	return true;
}

bool TSDocumentExtractor::ConstructTimeLineCollections(const TSQuery &query, TSTimeLineCollections &collections)
{
	auto probe = CProfiler::CProfilerProbe("constr_timeline_collections");

	TSDocCollection whole_collection;
	auto init_params = { (float)m_iDocCount, m_fSoftOr, m_fMinDocRank };
	if( m_pDataExtractor->GetDocuments(query, init_params, whole_collection) != ReturnCode::TS_NO_ERROR )
		return false;

	if( whole_collection.size() == 0 )
		return false;

	if( m_bTemporalMode ) {
		if( m_bDocImportance ) {
			std::map<std::string, float> doc_to_importance;
			std::vector<std::string> top_docs;
			ComputeDocsImportance(whole_collection, doc_to_importance, top_docs);

			if( !SeparateCollectionByTime(whole_collection, collections) )
				return false;

			collections.InitDocumentsImportanceData(std::move(doc_to_importance), std::move(top_docs));
		}
		else if( !SeparateCollectionByTime(whole_collection, collections) )
			return false;
	} else {
		while( whole_collection.size() > 0 ) {
			auto doc_iter = whole_collection.begin();
			collections.AddDocNode(whole_collection.ExtractNode(doc_iter->first), 0);
		}
	}

	return true;
}

bool TSDocumentExtractor::SeparateCollectionByTime(TSDocCollection &whole_collection, TSTimeLineCollections &collections) const
{
	auto probe = CProfiler::CProfilerProbe("separate_by_time");
	while( whole_collection.size() > 0 ) {
		auto doc_iter = whole_collection.begin();
		std::string s_int_date;
		if( !doc_iter->second.GetMetaData(SMetaDataType::INT_DATE, s_int_date) )
			return false;

		collections.AddDocNode(whole_collection.ExtractNode(doc_iter->first), std::stoi(s_int_date));
	}
	CLogger::Instance()->WriteToLog("INFO : collections days before cut = " + std::to_string(collections.size()));
	CutDaysWithSmallPublicationsSize(collections);
	CLogger::Instance()->WriteToLog("INFO : collections days after cut = " + std::to_string(collections.size()));

	return true;
}

void TSDocumentExtractor::CutDaysWithSmallPublicationsSize(TSTimeLineCollections &collections) const
{
	auto probe = CProfiler::CProfilerProbe("cut_collections");
	std::vector<int> coll_sizes;
	coll_sizes.reserve(collections.size());
	for( const auto &day_collection : collections ) {
		coll_sizes.push_back((int)day_collection.second.size());
	}

	std::sort(coll_sizes.begin(), coll_sizes.end(), std::greater<int>());
	int summ_top_3 = 0;
	for( int i = 0; i < min(3, coll_sizes.size()); i++ )
		summ_top_3 += coll_sizes[i];

	summ_top_3 /= min(3, (int)coll_sizes.size());

	collections.EraseCollectionsWithSizeLessThen((int)(summ_top_3 * 0.2f));
}

void TSDocumentExtractor::ComputeDocsImportance(const TSDocCollection &whole_collection, std::map<std::string, float> &doc_to_importance, std::vector<std::string> &top_docs) const
{
	auto probe = CProfiler::CProfilerProbe("compute_doc_importance");
	std::vector<TSDocumentRepresentation> docs_representations;
	ConstructDocumentsRepresentations(whole_collection, docs_representations);

	std::vector<std::vector<float>> similarity_matrix;
	ConstructSimilarityMatrix(docs_representations, similarity_matrix);

	std::vector<float> importances;
	PowerMethod(docs_representations, similarity_matrix, importances);

	for( int i = 0; i < importances.size(); i++ )
		docs_representations[i].SetImportance(importances[i]);

	std::sort(docs_representations.begin(), docs_representations.end(), [](const TSDocumentRepresentation &lhs, const TSDocumentRepresentation &rhs) {
		return lhs.GetImportance() > rhs.GetImportance();
	});

	for( const auto doc_repr : docs_representations ) {
		doc_to_importance.insert(doc_repr.GetPair());
		if( doc_repr.GetImportance() > m_fDocumentImportanceBoundary ) {
			//std::cout << doc_repr.GetDocPtr()->GetDocID() << " " << doc_repr.GetImportance() << std::endl;
			CLogger::Instance()->WriteToLog("INFO : topdoc = " + doc_repr.GetDocPtr()->GetDocID() + " i = " + std::to_string(doc_repr.GetImportance()));
			top_docs.push_back(doc_repr.GetDocPtr()->GetDocID());
		}
	}
	
}

void TSDocumentExtractor::ConstructSimilarityMatrix(const std::vector<TSDocumentRepresentation> &docs_representations, std::vector<std::vector<float>> &similarity_matrix) const
{
	auto probe = CProfiler::CProfilerProbe("construct_sim_matrix");
	auto int_date_by_hour = [] (const std::string &data) {
		std::string day = data.substr(0, 2);
		std::string month = data.substr(3, 2);
		std::string year = data.substr(6, 4);
		std::string hour = data.substr(11, 2);
		if( !utils::IsStringIntNumber(day) || !utils::IsStringIntNumber(month) || !utils::IsStringIntNumber(year) || !utils::IsStringIntNumber(hour) )
			return -1;

		return std::stoi(hour) + std::stoi(day) * 24 + std::stoi(month) * 24 * 31 + std::stoi(year) * 24 * 365;
	};

	int size = (int)docs_representations.size();
	similarity_matrix.resize(size);
	for( auto &row : similarity_matrix ) {
		row.resize(size);
		std::fill(row.begin(), row.end(), 0.f);
	}

	for( int i = 0; i < size; i++ ) {
		for( int j = 0; j < size; j++ ) {
			if( j == i )
				continue;
			// if reference from j to i
			if( int_date_by_hour(docs_representations[j].GetDate()) > int_date_by_hour(docs_representations[i].GetDate()) ) {
				float sim = docs_representations[j] * docs_representations[i];
				if( sim > m_fMinLinkScore )
					similarity_matrix[i][j] = sim;
			}
		}
	}
}

void TSDocumentExtractor::PowerMethod(const std::vector<TSDocumentRepresentation> &docs_representations, const std::vector<std::vector<float>> &similarity_matrix, std::vector<float> &importances) const
{
	auto probe = CProfiler::CProfilerProbe("power_method");
	int size = (int)docs_representations.size();
	importances.resize(size);

	std::vector<float> importance_next(size, m_fPowerDFactor / size), importance_last(size, m_fPowerDFactor / size);

	std::vector<int> degrees(size, 0);
	for( int i = 0; i < size; i++ )
		for( int j = 0; j < size; j++ )
			if( similarity_matrix[i][j] > 0.f )
				degrees[j]++;
	

	float max_dif = 1.f, epsilon = 0.00001f;

	while( max_dif > epsilon ) {
		max_dif = 0;
		// calculate importance_next
		for( int i = 0; i < size; i++ ) {
			importance_next[i] = m_fPowerDFactor / size;

			for( auto j = 0; j < size; j++ ) {
				if( similarity_matrix[i][j] > 0.f ) {
					importance_next[i] += (1.f - m_fPowerDFactor)  * importance_last[j] / degrees[j];
				}
			}

			float dif = std::fabs(importance_next[i] - importance_last[i]);
			max_dif = max(max_dif, dif);
			
		}
		importance_last.swap(importance_next);
	}

	float max_value = *std::max_element(importance_last.begin(), importance_last.end());
	if( max_value > m_fPowerDFactor / size ) {
		for( auto &elem : importance_last )
			elem /= max_value;

		importances.swap(importance_last);
	} else
		std::fill(importances.begin(), importances.end(), 0.f);
}

void TSDocumentExtractor::ConstructDocumentsRepresentations(const TSDocCollection &whole_collection, std::vector<TSDocumentRepresentation> &docs_representations) const
{
	docs_representations.resize(whole_collection.size());
	int i = 0;
	for( const auto &doc_pair : whole_collection ) {
		ConstructDocRepresentation(doc_pair.second, docs_representations[i]);
		i++;
	}
}

void TSDocumentExtractor::ConstructDocRepresentation(const TSDocument &document, TSDocumentRepresentation &doc_representation) const
{
	doc_representation.InitDocPtr(&document);

	int i = 0;
	for( auto sentences_iter = document.sentences_begin(); sentences_iter != document.sentences_end() && i < m_iDocHeadSize; sentences_iter++, i++ ) {
		TSSentenceConstPtr sentence_ptr = &(*sentences_iter);
		if( m_bIsW2V ) {
			TSIndexConstPtr index_ptr;
			if( !sentence_ptr->GetIndex(SDataType::LEMMA, index_ptr) || !index_ptr->ConstructIndexEmbedding(m_pModel) )
				continue;
		}
		doc_representation.AddHead(sentence_ptr);
	}

	i = 0;
	for( auto sentences_iter = document.sentences_rbegin(); sentences_iter != document.sentences_rend() && i < m_iDocTailSize; sentences_iter++, i++ ) {
		TSSentenceConstPtr sentence_ptr = &(*sentences_iter);
		if( IsSentenceHasReferenceToThePast(sentence_ptr) ) {
			if( m_bIsW2V ) {
				TSIndexConstPtr index_ptr;
				if( !sentence_ptr->GetIndex(SDataType::LEMMA, index_ptr) || !index_ptr->ConstructIndexEmbedding(m_pModel) )
					continue;
			}
			doc_representation.AddTail(sentence_ptr);
		}
	}
}

bool TSDocumentExtractor::IsSentenceHasReferenceToThePast(TSSentenceConstPtr sentence) const
{
	TSIndexConstPtr p_index;
	if( !sentence->GetIndex(SDataType::LEMMA, p_index) )
		return false;

	if( p_index->size() < 4 /*m_iMinSentenceSize*/ || p_index->size() < 30 /*m_MaxSentenceSize*/ )
		return false;

	return sentence->GetDocPtr()->sentences_size() - sentence->GetSentenseNumber() <= m_iDocTailSize;
}

void TSDocumentExtractor::TSDocumentRepresentation::InitDocPtr(TSDocumentConstPtr doc_ptr)
{
	m_pDoc = doc_ptr;
	m_pDoc->GetMetaData(SMetaDataType::DATE, m_sDate);

}

void TSDocumentExtractor::TSDocumentRepresentation::AddHead(TSSentenceConstPtr sentence_ptr)
{
	m_HeadSentences.push_back(sentence_ptr);
}

void TSDocumentExtractor::TSDocumentRepresentation::AddTail(TSSentenceConstPtr sentence_ptr)
{
	m_TailSentences.push_back(sentence_ptr);
}

float TSDocumentExtractor::TSDocumentRepresentation::operator*(const TSDocumentRepresentation &other) const
{
	float weight = 0.f;
	for( auto tail_sentence : m_TailSentences ) {
		for( auto head_sentence : other.m_HeadSentences ) {
			if( m_bIsW2V ) {
				weight = max(weight, tail_sentence->EmbeddingSimilarity(*head_sentence, SDataType::LEMMA));
			} else {
				weight = max(weight, *tail_sentence * *head_sentence);
			}
		}
	}
	return weight;
}