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

ReturnCode TSDataExtractor::InitParameters(const std::initializer_list<float> &params) const
{
	if( !m_spSearchEngine )
		return ReturnCode::TS_GENERAL_ERROR;

	if( !m_spSearchEngine->InitParameters(params) )
		return ReturnCode::TS_GENERAL_ERROR;

	return ReturnCode::TS_NO_ERROR;
}

ReturnCode TSDataExtractor::GetDocument(const std::string &doc_id, TSDocumentPtr document) const
{
	auto probe = CProfiler::CProfilerProbe("get_doc");
	if( !m_spSearchEngine || !m_pReplyProcessor ) {
		CLogger::Instance()->WriteToLog("ERROR: Search engine or reply processor does not inited");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	ReturnCode result = m_DataHistory.LoadDocument(doc_id, document);
	if( result == ReturnCode::TS_DOC_SKIPPED ) {
		//CLogger::Instance()->WriteToLog("SKIPPED: Doc " + doc_id);
		return ReturnCode::TS_DOC_SKIPPED;
	} else if( result == ReturnCode::TS_GENERAL_ERROR ) {
#pragma omp critical (data_extractor_get_doc) 
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
			SortDocIndexies(document);
			m_DataHistory.SaveDocument(doc_id, document);
		}
	}

	return ReturnCode::TS_NO_ERROR;
}

void TSDataExtractor::SortDocIndexies(TSDocumentPtr document) const
{
	for( long type = (long)SDataType::LEMMA; type != (long)SDataType::FINAL_TYPE; type++ ) {
		TSIndexPtr p_index;
		if( document->GetIndex((SDataType)type, p_index) ) {
			std::sort(p_index->begin(), p_index->end(), [] (const TSIndexItem &lhs, const TSIndexItem &rhs) {
				return lhs.GetWeight() > rhs.GetWeight();
			});
		}
	}
	for( auto sentence_iter = document->sentences_begin(); sentence_iter != document->sentences_end(); sentence_iter++ ) {
		for( long type = (long)SDataType::LEMMA; type != (long)SDataType::FINAL_TYPE; type++ ) {
			TSIndexPtr p_index;
			if( sentence_iter->GetIndex((SDataType)type, p_index) )
				std::sort(p_index->begin(), p_index->end());
		}
	}
}

bool TSDataExtractor::GetDocumentList(const TSQuery &query, std::vector<std::string> &doc_list) const
{
#pragma omp critical (data_extractor_get_doc_list) 
	{
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
	
}

ReturnCode TSDataExtractor::GetDocuments(const TSQuery &query, TSDocCollection &collection) const
{
	auto probe = CProfiler::CProfilerProbe("get_docs");
	if( !m_spSearchEngine || !m_pReplyProcessor ) {
		CLogger::Instance()->WriteToLog("ERROR: Search engine or reply processor does not inited");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	std::vector<std::string> doc_list;
	if( !GetDocumentList(query, doc_list) ) {
		CLogger::Instance()->WriteToLog("ERROR: Fail while getting document list on query");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	CLogger::Instance()->WriteToLog("Process " + std::to_string(doc_list.size()) + " docs");
	int counter = 0;
	for( const auto &doc_id : doc_list ) {
		if( counter++ % 10 )
			std::cout << "\r\t\t\t\t\r" << counter * 100.f / doc_list.size();

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

TSDataCollection::TSDataCollection() {
	if( !LoadData() )
		CLogger::Instance()->WriteToLog("ERROR : Failed while load docs names");
}
TSDataCollection::~TSDataCollection()
{
	if( !SaveData() )
		CLogger::Instance()->WriteToLog("ERROR : Failed while save docs names");
}

ReturnCode TSDataCollection::LoadDocument(const std::string &doc_id, TSDocumentPtr &doc_ptr)
{
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
	if( m_SavedDocs.find(doc_id) != m_SavedDocs.end() || m_FailedDocs.find(doc_id) != m_FailedDocs.end() )
		return true;

	HistoryController hist_controller(m_sSavedDocsPath);
	hist_controller.SaveMode(doc_id);
	doc_ptr->SaveToHistoryController(hist_controller);
	hist_controller.CloseFiles();
	m_SavedDocs.insert(doc_id);

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