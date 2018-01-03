#include "TSDataExtractor.h"
#include "../tshttpmanager.h"
#include <iostream>
#include "utils.h"
#include <regex>
#include <windows.h>
#include <locale>
#include <chrono>
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
	if( !m_spSearchEngine || !m_pReplyProcessor ) {
		CLogger::Instance()->WriteToLog("ERROR: Search engine or reply processor does not inited");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	RequestDataType request = doc_id;
	ReplyDataType reply;
	if( !m_spSearchEngine->SendRequest(request, reply) ) {
		CLogger::Instance()->WriteToLog("ERROR: Fail while sending request to search engine");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	if( reply.second.size() > m_iMaxDocSize ) {
		CLogger::Instance()->WriteToLog("SKIPPED: Doc " + doc_id + " , size = " + std::to_string(reply.second.size()));
		return ReturnCode::TS_DOC_SKIPPED;
	}

	ProcessedDataType data = document;
	if( !m_pReplyProcessor->ProcessReply(reply, data) ) {
		CLogger::Instance()->WriteToLog("ERROR: Fail while process reply from search engine");
		return ReturnCode::TS_GENERAL_ERROR;
	}

	document->InitDocID(doc_id);
	SortDocIndexies(document);

	return ReturnCode::TS_NO_ERROR;
}

void TSDataExtractor::SortDocIndexies(TSDocumentPtr document) const
{
	for( long type = (long)SDataType::LEMMA; type != (long)SDataType::FINAL_TYPE; type++ ) {
		TSIndexPtr p_index;
		document->GetIndex((SDataType)type, p_index);
		std::sort(p_index->begin(), p_index->end(), [] (const TSIndexItem &lhs, const TSIndexItem &rhs) {
			return lhs.GetWeight() > rhs.GetWeight();
		});
	}
}

bool TSDataExtractor::GetDocumentList(const TSQuery &query, std::vector<std::string> &doc_list) const
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

ReturnCode TSDataExtractor::GetDocuments(const TSQuery &query, TSDocCollection &collection) const
{
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
	for( const auto &doc_id : doc_list ) {
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