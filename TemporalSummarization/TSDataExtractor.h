#pragma once
#include "TSPrimitives.h"
#include <memory>

enum class ReturnCode {
	TS_NO_ERROR = 0,
	TS_GENERAL_ERROR = 1,
	TS_DOC_SKIPPED = 2,
};

class TSDataExtractor
{
public:
	TSDataExtractor();
	~TSDataExtractor();

	ReturnCode InitParameters(const std::initializer_list<float> &params) const;
	ReturnCode GetDocument(TSDocument &document, const std::string &doc_id) const;
	ReturnCode GetDocuments(TSDocCollection &collection, const TSQuery &query) const;

private:
	bool GetDocumentList(const TSQuery &query, std::vector<std::string> &doc_list) const;
	void SortDocIndexies(TSDocument &document) const;

private:
	std::unique_ptr<class ISearchEngine> m_spSearchEngine;
	class IReplyProcessor *m_pReplyProcessor;
	int m_iMaxDocSize;
};

