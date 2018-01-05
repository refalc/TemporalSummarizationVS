#include "TSSolver.h"
#include <algorithm>
#include <omp.h>
#include <numeric>

TSSolver::TSSolver() :
	m_iMaxDailyAnswerSize(10),
	m_fLambda(0.85f),
	m_fSimThreshold(0.8f)
{
}


TSSolver::~TSSolver()
{
}


bool TSSolver::InitParameters(const std::initializer_list<float> &params)
{
	constexpr int parameters_size = 3;
	if( params.size() < parameters_size )
		return false;

	int max_daily_size = (int)params.begin()[0];

	float lambda = params.begin()[1],
		  sim_threshold = params.begin()[2];

	if( max_daily_size < 1 ||
		lambda > 1 || lambda < 0 ||
		sim_threshold  > 1 || sim_threshold  < 0 )
		return false;


	m_iMaxDailyAnswerSize = max_daily_size;
	m_fLambda = lambda;
	m_fSimThreshold = sim_threshold;

	return true;
}

bool TSSolver::GetTemporalSummary(const TSTimeLineCollections &collections, const TSTimeLineQueries &queries, int sentences_number, std::vector<std::pair<float, TSSentenceConstPtr>> &sentences) const
{
	std::map<float, TSSentenceConstPtr> all_extracted;
	for( const auto &day_collection : collections ) {
		std::map<float, TSSentenceConstPtr> today_extacted;
		std::pair<float, TSSentenceConstPtr> sentence_pair;
		TSQueryConstPtr today_query;
		if( !queries.GetQuery(day_collection.first, today_query) )
			return false;

		while( today_extacted.size() < m_iMaxDailyAnswerSize && GetTopSentence(day_collection.second, *today_query, all_extracted, today_extacted, sentence_pair) ) {
			today_extacted.insert(std::move(sentence_pair));
		}

		while( !today_extacted.empty() )
			all_extracted.insert(today_extacted.extract(today_extacted.begin()->first));
	}

	int elements_num = std::min(sentences_number, (int)all_extracted.size());
	auto iter_begin = all_extracted.rbegin(), iter_end = all_extracted.rbegin();
	while( elements_num-- )
		iter_end++;
	sentences.assign(iter_begin, iter_end);

	return true;
}

bool TSSolver::GetTopSentence(const TSDocCollection &collection, const TSQuery &query, const std::map<float, TSSentenceConstPtr> &extracted_sentences, const std::map<float, TSSentenceConstPtr> &extracted_sentences_today, std::pair<float, TSSentenceConstPtr> &sentence_pair) const
{
	int all_sentences_size = 0;
	for( const auto &doc_pair : collection )
		all_sentences_size += (int)doc_pair.second.sentences_size();

	std::vector<float> scores(all_sentences_size, 0.);
	std::vector<TSSentenceConstPtr> sentences;
	sentences.reserve(all_sentences_size);

	for( auto doc_iter = collection.begin(); doc_iter != collection.end(); doc_iter++ )
		for( auto sent_iter = doc_iter->second.sentences_begin(); sent_iter != doc_iter->second.sentences_end(); sent_iter++ )
			sentences.push_back(&(*sent_iter));

#pragma omp parallel for
	for( int i = 0; i > collection.size(); i++ ) {
		scores[i] = RankOneSentence(sentences[i], query, extracted_sentences, extracted_sentences_today);
	}

	int index_max = (int)std::distance(scores.begin(), std::max_element(scores.begin(), scores.end()));
	sentence_pair.first = scores[index_max];
	sentence_pair.second = sentences[index_max];

	return true;
}

float TSSolver::RankOneSentence(const TSSentenceConstPtr &sentence, const TSQuery &query, const std::map<float, TSSentenceConstPtr> &extracted_sentences, const std::map<float, TSSentenceConstPtr> &extracted_sentences_today) const
{
	float score = 0, sim_to_query = 0, sim_to_extracted = 0, sim_to_extraxted_today = 0;

	sim_to_query = *sentence * query;
	
	for( const auto sent : extracted_sentences )
		sim_to_extracted = std::max(sim_to_extracted, *sent.second * *sentence);

	for( const auto sent : extracted_sentences_today )
		sim_to_extraxted_today = std::max(sim_to_extracted, *sent.second * *sentence);

	float sim_to_other = std::max(sim_to_extracted, sim_to_extraxted_today);
	if( sim_to_other > m_fSimThreshold )
		return 0.f;

	float sentence_num_penality = 1.f - 0.5f * sin((float)sentence->GetSentenseNumber() / sentence->GetDocPtr()->sentences_size());
	score = sentence_num_penality * m_fLambda * sim_to_query - (1.f - m_fLambda) * sim_to_other;

	return score;
}