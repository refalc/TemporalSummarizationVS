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
	if( !m_spSearchEngine || !m_pReplyProcessor )
		return false;

	RequestDataType request = doc_id;
	ReplyDataType reply;
	if( !m_spSearchEngine->SendRequest(request, reply) )
		return false;

	ProcessedDataType data;
	if( !m_pReplyProcessor->ProcessReply(reply, data) )
		return false;

	document.InitDocID(doc_id);
	return true;
}

/*std::string TSDataExtractor::ConstructDocListRequestString(const TSWordSequence &query) const
{
	return "";
}*/

/*bool TSDataExtractor::GetDocuments(TSDocCollection &collection, const TSWordSequence &query) const
{
	std::string request = m_sDocListRequestPart + ConstructDocListRequestString(query), reply;
	
	if( !m_spHttpManager->Get(request, reply) )
		return false;

	return true;
}*/