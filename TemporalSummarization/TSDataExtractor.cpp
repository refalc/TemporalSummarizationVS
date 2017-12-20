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
}


TSDataExtractor::~TSDataExtractor()
{

}

bool TSDataExtractor::InitParameters(const std::initializer_list<float> &params) const
{
	if( !m_spSearchEngine )
		return false;

	return m_spSearchEngine->InitParameters(params);
}

bool TSDataExtractor::GetDocument(TSDocument &document, const std::string &doc_id) const
{
	if( !m_spSearchEngine || !m_pReplyProcessor ) {
		CLogger::Instance()->WriteToLog("ERROR: Search engine or reply processor does not inited");
		return false;
	}

	RequestDataType request = doc_id;
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
	std::get<TSDocument>(data).InitDocID(doc_id);

	document = std::move(std::get<TSDocument>(data));

	return true;
}

bool TSDataExtractor::GetDocumentList(const std::vector<TSIndex> &query, std::vector<std::string> &doc_list) const
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

bool TSDataExtractor::GetDocuments(TSDocCollection &collection, const std::vector<TSIndex> &query) const
{
	if( !m_spSearchEngine || !m_pReplyProcessor ) {
		CLogger::Instance()->WriteToLog("ERROR: Search engine or reply processor does not inited");
		return false;
	}

	std::vector<std::string> doc_list;
	if( !GetDocumentList(query, doc_list) ) {
		CLogger::Instance()->WriteToLog("ERROR: Fail while getting document list on query");
		return false;
	}

	CLogger::Instance()->WriteToLog("Process " + std::to_string(doc_list.size()) + " docs");
	for( const auto &doc_id : doc_list ) {
		TSDocument doc(doc_id);
		if( !GetDocument(doc, doc.GetDocID()) )
			return false;

		collection.AddDocToCollection(std::move(doc));
	}

	return true;
}