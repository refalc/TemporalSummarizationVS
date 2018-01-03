#pragma once
#include "TSPrimitives.h"
#include <set>

class TSSolver : public ISimpleModule
{
public:
	TSSolver();
	~TSSolver();

	virtual bool InitParameters(const std::initializer_list<float> &params) override;
	bool GetTemporalSummary(const TSTimeLineCollections &collections, int sentence_number, std::vector<TSSentencePtr> &sentences) const;

private:
	bool GetTopSentence(const TSDocCollection &collection, const TSQuery &query, const std::set<TSSentencePtr> &extracted_sentences, const std::set<TSSentencePtr> &extracted_sentences_today, std::pair<float, TSSentencePtr> &sentence_pair) const;


private:
	int m_iMaxDailyAnswerSize;
};

