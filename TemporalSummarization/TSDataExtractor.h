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
	ReturnCode GetDocument(const std::string &doc_id, TSDocumentPtr document) const;
	ReturnCode GetDocuments(const TSQuery &query, TSDocCollection &collection) const;

private:
	bool GetDocumentList(const TSQuery &query, std::vector<std::string> &doc_list) const;
	void SortDocIndexies(TSDocumentPtr document) const;

private:
	std::unique_ptr<class ISearchEngine> m_spSearchEngine;
	class IReplyProcessor *m_pReplyProcessor;
	int m_iMaxDocSize;
};

