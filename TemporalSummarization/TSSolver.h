#pragma once
#include "TSPrimitives.h"
#include <set>

class TSSolver : public ISimpleModule
{
public:
	using TSSolverSentenceData = std::variant<TSSentenceConstPtr, std::pair<TSSentenceConstPtr, float>>;

	TSSolver();
	~TSSolver();

	virtual bool InitParameters(const std::initializer_list<float> &params) override;
	bool GetTemporalSummary(const TSTimeLineCollections &collections, const TSTimeLineQueries &queries, int sentence_number, std::vector<std::pair<float, TSSentenceConstPtr>> &sentences) const;

private:
	bool GetTopSentence(std::vector<TSSolverSentenceData> &collection, const TSQuery &query, const std::multimap<float, TSSentenceConstPtr> &extracted_sentences, const std::multimap<float, TSSentenceConstPtr> &extracted_sentences_today, std::pair<float, TSSentenceConstPtr> &sentence_pair) const;
	float RankOneSentence(const TSSolverSentenceData &sentence, const TSQuery &query, const std::multimap<float, TSSentenceConstPtr> &extracted_sentences, const std::multimap<float, TSSentenceConstPtr> &extracted_sentences_today) const;
	bool CreateSentencesFromCollection(const TSTimeLineCollections &collections, const TSDocCollection &collection, std::vector<TSSolverSentenceData> &sentences) const;

private:
	int m_iMaxDailyAnswerSize;
	float m_fSimThreshold;
	float m_fLambda;
	float m_fMinMMR;
	int m_iMinSentenceSize;
	int m_iMaxSentenceSize;
	bool m_bDocImportance;
	float m_fAlpha;
	bool m_bIsW2V;
};

