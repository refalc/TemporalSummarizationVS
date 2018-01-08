#pragma once
#include "TSPrimitives.h"
#include <memory>
#include <set>

class CNldxSearchEngine : public ISearchEngine
{
public:
	CNldxSearchEngine();
	virtual ~CNldxSearchEngine();
	virtual bool InitParameters(const std::initializer_list<float> &params) override;
	virtual bool SendRequest(const RequestDataType &request, ReplyDataType &reply) override;
	virtual IReplyProcessor *GetReplyProcessor() override { return &m_ReplyProcessor; }

private:
	std::string ConstructDocListRequestString(const TSQuery &query) const;

	class CNldxReplyProcessor : public IReplyProcessor {
	public:
		CNldxReplyProcessor();
		virtual ~CNldxReplyProcessor() {}
		virtual bool ProcessReply(const ReplyDataType &reply, ProcessedDataType &data) const override;
		bool InitParameters(const std::initializer_list<float> &params);
	private:
		void InitStopWords();
		bool ConstructDocFromString(TSDocumentPtr document, std::string &&doc_text) const;
		bool ConstructDocListFromString(std::vector<std::string> &doc_list, std::string &&doc_text) const;
		bool ExtractMetaData(TSDocumentPtr document, const std::string &doc_text) const;
		SDataType String2Type(const std::string &str) const;
		bool ExtractIndexData(TSDocumentPtr document, const std::string &doc_text) const;
		bool FindTag(const std::string &text, const std::string &open_tag, const std::string &close_tag, int left_boundary, int right_boundary, std::pair<int, int> &result) const;
		bool ProcessTrData(TSDocumentPtr document, const std::string &stype, const std::string &sword, const std::string &sweight, const std::string &scount, const std::string &spos_data) const;
		std::vector<std::array<int, 3>> ProcessPosData(const std::string &str, int count) const;
		void CutSentencesWithHDRTag(TSDocumentPtr document) const;
	private:
		float m_fMinDocRank;
		//std::set<std::string> m_StopWords;
		std::vector<std::string> m_StopWords;
	};
private:

	std::string m_sDocListRequestPart;
	std::string m_sDocRequestPart;

	std::unique_ptr<class TSHttpManager> m_spHttpManager;
	CNldxReplyProcessor m_ReplyProcessor;
};

