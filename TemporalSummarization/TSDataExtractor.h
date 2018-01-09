#pragma once
#include "TSPrimitives.h"
#include <memory>

enum class ReturnCode {
	TS_NO_ERROR = 0,
	TS_GENERAL_ERROR = 1,
	TS_DOC_SKIPPED = 2,
};

class TSDataCollection
{
public:
	TSDataCollection();
	~TSDataCollection();

	bool LoadData();
	bool SaveData() const;
	ReturnCode LoadDocument(const std::string &doc_id, TSDocumentPtr &doc_ptr);
	bool SaveDocument(const std::string &doc_id, TSDocumentPtr &doc_ptr);
	bool AddToFailedList(const std::string &doc_id);


private:
	const std::string m_sSavedDocsPath = "C:/!DEV/C++/TemporalSummarization/TemporalSummarizationVS/Data/Docs/";
	const std::string m_sFailedDocsFileName = "failed_docs.txt";
	const std::string m_sSavedDocsFileName = "saved_docs.txt";

	std::set<std::string> m_SavedDocs;
	std::set<std::string> m_FailedDocs;

	mutable HistoryController m_HistoryController;
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
	mutable TSDataCollection m_DataHistory;
};

