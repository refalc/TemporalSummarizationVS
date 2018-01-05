#pragma once
#include "TSPrimitives.h"
#include <set>

class TSSolver : public ISimpleModule
{
public:
	TSSolver();
	~TSSolver();

	virtual bool InitParameters(const std::initializer_list<float> &params) override;
	bool GetTemporalSummary(const TSTimeLineCollections &collections, const TSTimeLineQueries &queries, int sentence_number, std::vector<std::pair<float, TSSentenceConstPtr>> &sentences) const;

private:
	bool GetTopSentence(const TSDocCollection &collection, const TSQuery &query, const std::map<float, TSSentenceConstPtr> &extracted_sentences, const std::map<float, TSSentenceConstPtr> &extracted_sentences_today, std::pair<float, TSSentenceConstPtr> &sentence_pair) const;
	float RankOneSentence(const TSSentenceConstPtr &sentence, const TSQuery &query, const std::map<float, TSSentenceConstPtr> &extracted_sentences, const std::map<float, TSSentenceConstPtr> &extracted_sentences_today) const;

private:
	int m_iMaxDailyAnswerSize;
	float m_fSimThreshold;
	float m_fLambda;
};

