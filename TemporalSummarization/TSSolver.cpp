#include "TSSolver.h"
#include <algorithm>
#include <omp.h>
#include <numeric>
#include <chrono>
#include "utils.h"

TSSolver::TSSolver() :
	m_iMaxDailyAnswerSize(10),
	m_fLambda(0.85f),
	m_fSimThreshold(0.8f),
	m_iMinSentenceSize(4),
	m_iMaxSentenceSize(35)
{
}


TSSolver::~TSSolver()
{
}


bool TSSolver::InitParameters(const std::initializer_list<float> &params)
{
	constexpr int parameters_size = 6;
	if( params.size() < parameters_size )
		return false;

	int max_daily_size = (int)params.begin()[0];

	float lambda = params.begin()[1],
		  sim_threshold = params.begin()[2],
		  min_mmr = params.begin()[3];

	bool doc_importance = (bool)params.begin()[4];

	float alpha = params.begin()[5];

	bool is_w2v = (bool)params.begin()[6];

	if( max_daily_size < 1 ||
		lambda > 1 || lambda < 0 ||
		sim_threshold  > 1 || sim_threshold  < 0||
		min_mmr > 1 || min_mmr < 0 ||
		alpha < 0 )
		return false;


	m_iMaxDailyAnswerSize = max_daily_size;
	m_fLambda = lambda;
	m_fSimThreshold = sim_threshold;
	m_fMinMMR = min_mmr;
	m_bDocImportance = doc_importance;
	m_fAlpha = alpha;
	m_bIsW2V = is_w2v;

	return true;
}

bool TSSolver::GetTemporalSummary(const TSTimeLineCollections &collections, const TSTimeLineQueries &queries, int sentences_number, std::vector<std::pair<float, TSSentenceConstPtr>> &sentences) const
{
	auto probe = CProfiler::CProfilerProbe("slv");
	std::multimap<float, TSSentenceConstPtr> all_extracted;
	for( const auto &day_collection : collections ) {
		std::multimap<float, TSSentenceConstPtr> today_extacted;
		std::pair<float, TSSentenceConstPtr> sentence_pair;
		TSQueryConstPtr today_query;
		if( !queries.GetQuery(day_collection.first, today_query) ) {
			return false;
		}

		std::vector<TSSolverSentenceData> sentences;
		if( !CreateSentencesFromCollection(collections, day_collection.second, sentences) )
			return false;

		while( today_extacted.size() < m_iMaxDailyAnswerSize && !sentences.empty() && GetTopSentence(sentences, *today_query, all_extracted, today_extacted, sentence_pair) ) {
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

bool TSSolver::CreateSentencesFromCollection(const TSTimeLineCollections &collections, const TSDocCollection &collection, std::vector<TSSolverSentenceData> &sentences) const
{
	int all_sentences_size = 0;
	for( const auto &doc_pair : collection )
		all_sentences_size += (int)doc_pair.second.sentences_size();

	sentences.reserve(all_sentences_size);

	for( auto doc_iter = collection.begin(); doc_iter != collection.end(); doc_iter++ ) {
		for( auto sent_iter = doc_iter->second.sentences_begin(); sent_iter != doc_iter->second.sentences_end(); sent_iter++ ) {
			TSIndexConstPtr p_index;
			if( !sent_iter->GetIndex(SDataType::LEMMA, p_index) )
				continue;

			if( m_bIsW2V )
				if( !p_index->ConstructIndexEmbedding(m_pModel) )
					continue;

			int sentence_lemm_size = 0;
			for( const auto &lemm_iter : *p_index )
				sentence_lemm_size += (int)lemm_iter.GetPositions().size();

			if( sentence_lemm_size >= m_iMinSentenceSize && sentence_lemm_size <= m_iMaxSentenceSize ) {
				if( m_bDocImportance )
					sentences.push_back(std::make_pair(&(*sent_iter), collections.GetDocImportance(sent_iter->GetDocPtr()->GetDocID())));
				else 
					sentences.push_back(&(*sent_iter));
			}
		}
	}

	if( sentences.empty() )
		return false;

	return true;
}

bool TSSolver::GetTopSentence(std::vector<TSSolverSentenceData> &collection, const TSQuery &query, const std::multimap<float, TSSentenceConstPtr> &extracted_sentences, const std::multimap<float, TSSentenceConstPtr> &extracted_sentences_today, std::pair<float, TSSentenceConstPtr> &sentence_pair) const
{
	std::vector<float> scores(collection.size(), 0.);

#pragma omp parallel for
	for( int i = 0; i < collection.size(); i++ ) {
		scores[i] = RankOneSentence(collection[i], query, extracted_sentences, extracted_sentences_today);
	}

	int index_max = (int)std::distance(scores.begin(), std::max_element(scores.begin(), scores.end()));
	if( scores[index_max] <= m_fMinMMR )
		return false;

	sentence_pair.first = scores[index_max];
	if( m_bDocImportance )
		sentence_pair.second = std::get<std::pair<TSSentenceConstPtr, float>>(collection[index_max]).first;
	else
		sentence_pair.second = std::get<TSSentenceConstPtr>(collection[index_max]);

	auto placer_iter = collection.begin(), runner_iter = collection.begin();
	for( int i = 0; i < scores.size(); i++ ) {
		if( scores[i] > m_fMinMMR ) {
			*placer_iter = *runner_iter;
			placer_iter++;
		}
		runner_iter++;
	}
	if( placer_iter != runner_iter )
		collection.erase(placer_iter, collection.end());

	return true;
}

float TSSolver::RankOneSentence(const TSSolverSentenceData &sentence, const TSQuery &query, const std::multimap<float, TSSentenceConstPtr> &extracted_sentences, const std::multimap<float, TSSentenceConstPtr> &extracted_sentences_today) const
{
	float score = 0, sim_to_query = 0, sim_to_extracted = 0, sim_to_extraxted_today = 0;


	TSSentenceConstPtr sentence_ptr;
	if( m_bDocImportance ) {
		std::pair<TSSentenceConstPtr, float> sentence_data_pair = std::get<std::pair<TSSentenceConstPtr, float>>(sentence);
		sentence_ptr = sentence_data_pair.first;
		if( m_bIsW2V )
			sim_to_query = sentence_ptr->EmbeddingSimilarity(query, SDataType::LEMMA);
		else
			sim_to_query = (*sentence_ptr * query);

		float doc_importance = sentence_data_pair.second;
		sim_to_query *= (1.f + m_fAlpha * doc_importance);
	} else {
		sentence_ptr = std::get<TSSentenceConstPtr>(sentence);
		if( m_bIsW2V )
			sim_to_query = sentence_ptr->EmbeddingSimilarity(query, SDataType::LEMMA);
		else
			sim_to_query = (*sentence_ptr * query);
	}
	
	if( m_bIsW2V ) {
		for( const auto sent : extracted_sentences )
			sim_to_extracted = std::max(sim_to_extracted, sent.second->EmbeddingSimilarity(*sentence_ptr, SDataType::LEMMA));

		for( const auto sent : extracted_sentences_today )
			sim_to_extraxted_today = std::max(sim_to_extraxted_today, sent.second->EmbeddingSimilarity(*sentence_ptr, SDataType::LEMMA));
	} else {
		for( const auto sent : extracted_sentences )
			sim_to_extracted = std::max(sim_to_extracted, *sent.second * *sentence_ptr);

		for( const auto sent : extracted_sentences_today )
			sim_to_extraxted_today = std::max(sim_to_extraxted_today, *sent.second * *sentence_ptr);
	}

	float sim_to_other = std::max(sim_to_extracted, sim_to_extraxted_today);
	if( sim_to_other > m_fSimThreshold )
		return 0.f;

	float sentence_num_penality = 1.f - 0.5f * sin((float)sentence_ptr->GetSentenseNumber() / sentence_ptr->GetDocPtr()->sentences_size());
	score = sentence_num_penality * m_fLambda * sim_to_query - (1.f - m_fLambda) * sim_to_other;

	return score;
}
