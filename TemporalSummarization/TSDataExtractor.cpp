#include "TSDataExtractor.h"
#include "../tshttpmanager.h"
#include <iostream>
#include "utils.h"
#include <regex>
#include <windows.h>
#include <locale>
#include "CNldxSearchEngine.h"

TSDataExtractor::TSDataExtractor()
{
	m_spSearchEngine.reset(new CNldxSearchEngine);
	m_pReplyProcessor = m_spSearchEngine->GetReplyProcessor();
	m_iMaxDocSize = 500000;
}


TSDataExtractor::~TSDataExtractor()
{

}
ReturnCode TSDataExtractor::InitDocsSerializationPath(const std::string &docs_serialization_path)
{
	if( !m_DataHistory.InitDocsSerializationPath(docs_serialization_path) )
		return ReturnCode::TS_GENERAL_ERROR;

	return ReturnCode::TS_NO_ERROR;
}

ReturnCode TSDataExtractor::InitParameters(const std::initializer_list<float> &params) const
{
	if( !m_spSearchEngine )
		return ReturnCode::TS_GENERAL_ERROR;

	if( !m_spSearchEngine->InitParameters(params) )
		return ReturnCode::TS_GENERAL_ERROR;

	return ReturnCode::TS_NO_ERROR;
}

ReturnCode TSDataExtractor::InitTopicModel(TopicModel *ptm)
{
	m_pTopicModel = ptm;
	return ReturnCode::TS_NO_ERROR;;
}

ReturnCode TSDataExtractor::GetDocument(const std::string &doc_id, TSDocumentPtr document) const
{
	auto probe = CProfiler::CProfilerProbe("get_doc");
	if( !m_spSearchEngine || !m_pReplyProcessor ) {
		CLogger::Instance()->WriteToLog("ERROR: Search engine or reply processor does not inited");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	ReturnCode load_result = m_DataHistory.LoadDocument(doc_id, document), recv_result = ReturnCode::TS_GENERAL_ERROR;
	if( load_result == ReturnCode::TS_GENERAL_ERROR ) {
#pragma omp critical (data_extractor_get) 
		{
			load_result = m_DataHistory.LoadDocument(doc_id, document);
			if( load_result == ReturnCode::TS_GENERAL_ERROR ) {
				recv_result = RecvDocument(doc_id, document);
				SortDocIndexies(document);
			} 
		}
	} 
	
	ReturnCode ret_res;
	bool is_topic_modeling = m_pTopicModel != nullptr;
	//__debugbreak();
	if( load_result == ReturnCode::TS_NO_ERROR || load_result == ReturnCode::TS_DOC_SKIPPED ) {
		ret_res = load_result;
	} else if( load_result == ReturnCode::TS_GENERAL_ERROR ) {
		if( recv_result == ReturnCode::TS_NO_ERROR )
			m_DataHistory.SaveDocument(doc_id, document);

		ret_res = recv_result;
	}
	if( doc_id == "11086390" || doc_id == "11086396" || doc_id == "11086397" || doc_id == "11087140") {
		std::cout << "found\n";
	}
	//document->m_DocID == "11086390" || document->m_DocID == "11086396" || document->m_DocID == "11086397"
	if( is_topic_modeling && ret_res == ReturnCode::TS_NO_ERROR ) {
		auto temporal_id_mapping = [] (TSDocumentPtr document) {
			std::string id, file;
			if( !document->GetMetaData(SMetaDataType::ID, id) || !document->GetMetaData(SMetaDataType::FILE, file) )
				return -1;
			std::regex re_file("KRMN_(.*?)-(.*?)-R(.*?)");
			std::smatch content_match;
			if( !std::regex_search(file, content_match, re_file) )
				return -1;
			std::string date = content_match[1],
				sub_id = content_match[2];
			
			int mod = std::pow(2, 31) - 1;
			int idate = std::stoi(date.substr(0, 4));
			int isubid = std::stoi(sub_id);
			std::string sfullid = std::to_string(idate + isubid) + id;
			std::string sfullid10 = sfullid.size() > 10 ? sfullid.substr(sfullid.size() - 10, 10) : sfullid;
			long long llfullid = std::stoll(sfullid10);
			int magic_id = llfullid % mod;
		
			return magic_id;
		};

		int doc_ptm_id = temporal_id_mapping(document);
		std::vector<TSIndexItem> ptm_data;
		if( !m_pTopicModel->GetDataForDoc(doc_ptm_id, ptm_data) )
			__debugbreak();
		else {
			for( int i = 0; i < ptm_data.size(); i++ )
				document->AddIndexItemToIndex(SDataType::TOPIC_MODEL, std::move(ptm_data[i]));

			TSIndexPtr p_index;
			if( document->GetIndex(SDataType::TOPIC_MODEL, p_index) ) {
				std::sort(p_index->begin(), p_index->end());
			}
			else
				__debugbreak();
		}
	}
	return ret_res;
}

ReturnCode TSDataExtractor::RecvDocument(const std::string &doc_id, TSDocumentPtr document) const
{
	auto probe = CProfiler::CProfilerProbe("recv_doc");
	RequestDataType request = doc_id;
	ReplyDataType reply;
	if( !m_spSearchEngine->SendRequest(request, reply) ) {
		CLogger::Instance()->WriteToLog("ERROR: Fail while sending request to search engine");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	if( reply.second.size() > m_iMaxDocSize ) {
		//CLogger::Instance()->WriteToLog("SKIPPED: Doc " + doc_id + " , size = " + std::to_string(reply.second.size()));
		m_DataHistory.AddToFailedList(doc_id);
		return ReturnCode::TS_DOC_SKIPPED;
	}

	ProcessedDataType data = document;
	if( !m_pReplyProcessor->ProcessReply(reply, data) ) {
		CLogger::Instance()->WriteToLog("ERROR: Fail while process reply from search engine");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	document->InitDocID(doc_id);
	return ReturnCode::TS_NO_ERROR;
}

void TSDataExtractor::SortDocIndexies(TSDocumentPtr document) const
{
	// index must be sorted by term_id
	//__debugbreak();
	auto MergeSimilarLemmas = [] (TSIndexPtr &index) {
		if( index->GetType() != SDataType::LEMMA || index->size() < 2 )
			return;
		std::map<int, std::vector<TSIndexItem>::iterator> pos2item;
		for( auto iter = index->begin(); iter != index->end(); iter++ ) {
			for( auto pos : iter->GetPositions() ) {
				auto pos_iter = pos2item.find(pos);
				if( pos_iter == pos2item.end() ) {
					pos2item[pos] = iter;
				} else if( pos_iter->second->GetWeight() < iter->GetWeight() ) {
					pos_iter->second = iter;
				}
			}
		}

		std::set<TSIndexItem> items;
		for( auto pos_elem : pos2item )
			items.insert(std::move(*pos_elem.second));

		int i = 0;
		for( auto item : items ) {
			*(index->begin() + i) = item;
			i++;
		}
		index->erase(index->begin() + i, index->end());
	};

	for( long type = (long)SDataType::LEMMA; type != (long)SDataType::FINAL_TYPE; type++ ) {
		TSIndexPtr p_index;
		if( document->GetIndex((SDataType)type, p_index) ) {
			MergeSimilarLemmas(p_index);
			std::sort(p_index->begin(), p_index->end(), [] (const TSIndexItem &lhs, const TSIndexItem &rhs) {
				return lhs.GetWeight() > rhs.GetWeight();
			});
		}
	}
	for( auto sentence_iter = document->sentences_begin(); sentence_iter != document->sentences_end(); sentence_iter++ ) {
		for( long type = (long)SDataType::LEMMA; type != (long)SDataType::FINAL_TYPE; type++ ) {
			TSIndexPtr p_index;
			if( sentence_iter->GetIndex((SDataType)type, p_index) ) {
				MergeSimilarLemmas(p_index);
				std::sort(p_index->begin(), p_index->end());
			}
		}
	}
}

bool TSDataExtractor::GetDocumentList(const TSQuery &query, const std::initializer_list<float> &params, std::vector<std::string> &doc_list) const
{
	if( InitParameters(params) != ReturnCode::TS_NO_ERROR )
		return false;

	RequestDataType request = query;
	ReplyDataType reply;
	if( !m_spSearchEngine->SendRequest(request, reply) ) {
		CLogger::Instance()->WriteToLog("ERROR: Fail while sending request to search engine");
		return false;
	}

	ProcessedDataType data;
	if( !m_pReplyProcessor->ProcessReply(reply, data) ) {
		CLogger::Instance()->WriteToLog("ERROR: Fail while process reply from search engine");
		return false;
	}

	doc_list = std::move(std::get<std::vector<std::string> >(data));
	return true;
}

ReturnCode TSDataExtractor::GetDocuments(const TSQuery &query, const std::initializer_list<float> &params, TSDocCollection &collection) const
{
	auto probe = CProfiler::CProfilerProbe("get_docs");
	if( !m_spSearchEngine || !m_pReplyProcessor ) {
		CLogger::Instance()->WriteToLog("ERROR: Search engine or reply processor does not inited");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	std::vector<std::string> doc_list;

	bool result;
#pragma omp critical (data_extractor_get) 
	{
		result = GetDocumentList(query, params, doc_list);
	}
	if( !result ) {
		CLogger::Instance()->WriteToLog("ERROR: Fail while getting document list on query");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	CLogger::Instance()->WriteToLog("Process " + std::to_string(doc_list.size()) + " docs");
	int counter = 0;
	for( const auto &doc_id : doc_list ) {
		//if( counter++ % 10 )
			//std::cout << "\r\t\t\t\t\r" << counter * 100.f / doc_list.size();
		if( collection.CheckDocID(doc_id) )
			continue;
		TSDocumentPtr doc = collection.AllocateDocument();
		ReturnCode res = GetDocument(doc_id, doc);
		if( res == ReturnCode::TS_DOC_SKIPPED )
			continue;

		if( res != ReturnCode::TS_NO_ERROR )
			return ReturnCode::TS_GENERAL_ERROR;

		collection.CommitAllocatedDocument();
	}

	return ReturnCode::TS_NO_ERROR;
}

TSDataCollection::TSDataCollection() 
{
}
TSDataCollection::~TSDataCollection()
{
	if( !SaveData() )
		CLogger::Instance()->WriteToLog("ERROR : Failed while save docs names");
}

bool TSDataCollection::InitDocsSerializationPath(const std::string &docs_serialization_path)
{
	if( docs_serialization_path.empty() ) {
		CLogger::Instance()->WriteToLog("ERROR : Failed while InitDocsSerializationPath");
		return false;
	}

	m_sSavedDocsPath = docs_serialization_path;
	if( !LoadData() ) {
		CLogger::Instance()->WriteToLog("ERROR : Failed while load docs names");
		return false;
	}
	return true;
}

ReturnCode TSDataCollection::LoadDocument(const std::string &doc_id, TSDocumentPtr &doc_ptr)
{
	if( m_sSavedDocsPath.empty() )
		return ReturnCode::TS_GENERAL_ERROR;

	auto probe = CProfiler::CProfilerProbe("load_doc");
	if( m_FailedDocs.find(doc_id) != m_FailedDocs.end() )
		return ReturnCode::TS_DOC_SKIPPED;

	if( m_SavedDocs.find(doc_id) == m_SavedDocs.end() )
		return ReturnCode::TS_GENERAL_ERROR;

	HistoryController hist_controller(m_sSavedDocsPath);
	hist_controller.LoadMode(doc_id);
	doc_ptr->LoadFromHistoryController(hist_controller);
	hist_controller.CloseFiles();

	return ReturnCode::TS_NO_ERROR;
}

bool TSDataCollection::SaveDocument(const std::string &doc_id, TSDocumentPtr &doc_ptr)
{
	if( m_SavedDocs.find(doc_id) == m_SavedDocs.end() && m_FailedDocs.find(doc_id) == m_FailedDocs.end() ) {
#pragma omp critical (data_collection_save_doc) 
		{
		if( m_SavedDocs.find(doc_id) == m_SavedDocs.end() && m_FailedDocs.find(doc_id) == m_FailedDocs.end() ) {
			HistoryController hist_controller(m_sSavedDocsPath);
			hist_controller.SaveMode(doc_id);
			doc_ptr->SaveToHistoryController(hist_controller);
			hist_controller.CloseFiles();
			m_SavedDocs.insert(doc_id);
		}
		}
	}
	return true;
}

bool TSDataCollection::AddToFailedList(const std::string &doc_id)
{
	m_FailedDocs.insert(doc_id);
	return true;
}

bool TSDataCollection::LoadData()
{
	std::fstream pFile;
	pFile.open(m_sSavedDocsPath + m_sSavedDocsFileName, std::fstream::in);
	if( !pFile.is_open() ) {
		CLogger::Instance()->WriteToLog("ERROR : while open " + m_sSavedDocsPath + m_sSavedDocsFileName);
		return false;
	}

	std::string str;
	while( std::getline(pFile, str) ) {
		m_SavedDocs.emplace_hint(m_SavedDocs.end(), str);
	}

	pFile.close();

	pFile.open(m_sSavedDocsPath + m_sFailedDocsFileName, std::fstream::in);
	if( !pFile.is_open() ) {
		CLogger::Instance()->WriteToLog("ERROR : while open " + m_sSavedDocsPath + m_sFailedDocsFileName);
		return false;
	}

	while( std::getline(pFile, str) ) {
		m_FailedDocs.emplace_hint(m_FailedDocs.end(), str);
	}

	pFile.close();
	return true;
}

bool TSDataCollection::SaveData() const
{
	std::fstream pFile;
	pFile.open(m_sSavedDocsPath + m_sSavedDocsFileName, std::fstream::out);
	if( !pFile.is_open() )
		return false;

	for( const auto &elem : m_SavedDocs )
		pFile << elem << std::endl;

	pFile.close();

	pFile.open(m_sSavedDocsPath + m_sFailedDocsFileName, std::fstream::out);
	if( !pFile.is_open() )
		return false;

	for( const auto &elem : m_FailedDocs )
		pFile << elem << std::endl;

	pFile.close();
	return true;
}