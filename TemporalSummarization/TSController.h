#pragma once
#include "utils.h"
#include "TSPrimitives.h"
#include <memory>

class TSDataExtractor;
class TSQueryConstructor;
class TSDocumentExtractor;
class TSSolver;


class TSController
{
public:
	TSController();
	~TSController();
	bool InitParameters(const Params &params, const std::string &answer_path, const std::string &docs_serialization_path, const std::string &w2v_path, int summary_size);
	bool RunQueries(const std::vector<std::string> &queries) const;

private:
	bool RunQuery(const std::string &doc_id) const;

private:
	// todo to new modules
	bool SaveTemporalSummaryInFile(const std::vector<std::pair<float, TSSentenceConstPtr>> &temporal_summary, const TSTimeLineQueries &queries, const std::string &init_doc_id) const;
	bool CleanAnswerFile() const;

	bool ConstructTimeLinesQueries(TSQuery &&init_query, const std::string &init_doc_id, const TSTimeLineCollections &collections, TSTimeLineQueries &queries) const;
	int GetQueryDate(const std::string &init_doc_id, const TSTimeLineCollections &collections) const;

private:
	std::unique_ptr<TSDataExtractor> m_spDataExtractor;
	std::unique_ptr<TSQueryConstructor> m_spQueryConstructor;
	std::unique_ptr<TSDocumentExtractor> m_spDocExtractor;
	std::unique_ptr<TSSolver> m_spSolver;

	Params m_Params;
	std::string m_AnswerPath;
	std::string m_W2VPath;
	int m_iTemporalSummarySize;
	mutable int m_iStoryIDCounter;
	std::unique_ptr<Word2Vec> m_spModel;
};