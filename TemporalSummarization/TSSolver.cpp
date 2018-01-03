#include "TSSolver.h"



TSSolver::TSSolver()
{
}


TSSolver::~TSSolver()
{
}


bool TSSolver::InitParameters(const std::initializer_list<float> &params)
{

}

bool TSSolver::GetTemporalSummary(const TSTimeLineCollections &collections, int sentence_number, std::vector<TSSentencePtr> &sentences) const
{
	std::set<TSSentencePtr> all_extracted;
	for( const auto &day_collection : collections ) {
		std::set<TSSentencePtr> today_extacted;
		std::pair<float, TSSentencePtr> sentence_pair;
		while( GetTopSentence(day_collection.second, all_extracted, today_extacted, sentence_pair) && today_extacted.size() < m_iMaxDailyAnswerSize ) {
			today_extacted.
		}
	}
}

bool TSSolver::GetTopSentence(const TSDocCollection &collection, const TSQuery &query, const std::set<TSSentencePtr> &extracted_sentences, const std::set<TSSentencePtr> &extracted_sentences_today, std::pair<float, TSSentencePtr> &sentence_pair) const
{

}
