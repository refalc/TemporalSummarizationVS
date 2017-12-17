#pragma once
#include "TSPrimitives.h"
#include <memory>

class TSDataExtractor
{
public:
	TSDataExtractor();
	~TSDataExtractor();

	bool InitParameters(double soft_or, int doc_count);
	bool GetDocument(TSDocument &document, const std::string &doc_id) const;
	bool GetDocuments(TSDocCollection &collection, const TSWordSequence &query) const;

private:
	std::string ConstructDocListRequestString(const TSWordSequence &query) const;
	/*std::string CreateDocListRequest(const TSWordSequence &query);
	std::string CreateDocRequest(const std::string &doc_id);*/
	bool ConstructDocFromString(TSDocument &document, const std::string &doc_text) const;


private:
	std::string m_sDocListRequestPart;
	std::string m_sDocRequestPart;

	std::unique_ptr<class TSHttpManager> m_spHttpManager;
};

