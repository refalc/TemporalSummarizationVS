#pragma once
#include "TSPrimitives.h"
#include <memory>

class TSDataExtractor
{
public:
	TSDataExtractor();
	~TSDataExtractor();

	bool InitParameters(const std::initializer_list<float> &params) const;
	bool GetDocument(TSDocument &document, const std::string &doc_id) const;
	//bool GetDocuments(TSDocCollection &collection, const TSWordSequence &query) const;

private:
	std::unique_ptr<class ISearchEngine> m_spSearchEngine;
	class IReplyProcessor *m_pReplyProcessor;
};

