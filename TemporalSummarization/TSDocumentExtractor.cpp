#include "TSDocumentExtractor.h"
#include "TSDataExtractor.h"
#include <iostream>
#include <algorithm>
#include <numeric>

#define _USE_MATH_DEFINES
#include <math.h>

int TSDocumentExtractor::m_iGraphsCount = 0;

TSDocumentExtractor::TSDocumentExtractor() :
	m_iDocTailSize(4),
	m_iDocHeadSize(3),
	m_fClusterizationSimThreshold(0.25f),
	m_fMaxHourDiff(12.f),
	m_bClusterization(true),
	m_bPyramidFeature(true),
	m_bLexRank(true),
	m_iTopKValue(20)
{
}


TSDocumentExtractor::~TSDocumentExtractor()
{
}

bool TSDocumentExtractor::InitParameters(const std::initializer_list<float> &params)
{
	constexpr int parameters_size = 15;
	if( params.size() < parameters_size )
		return false;

	int doc_count = (int)params.begin()[0];
	float soft_or = params.begin()[1],
		  min_doc_rank = params.begin()[2],
		  min_link_score = params.begin()[3],
		  power_d_factor = params.begin()[4],
		  importance_boundary = params.begin()[5];
	bool is_doc_importance = (bool)params.begin()[6],
		 is_temporal = (bool)params.begin()[7],
		 is_w2v = (bool)params.begin()[8],
		 is_clusterization = (bool)params.begin()[9],
		 is_pyramid = (bool)params.begin()[10],
		 is_lexrank = (bool)params.begin()[11];

	float clusterization_sim_threshold = params.begin()[12],
		  max_hour_diff = params.begin()[13];

	int top_k_value = params.begin()[14];

	if( doc_count < 0 || soft_or < 0.0f || soft_or > 1.f ||
		min_doc_rank < 0.0f || min_doc_rank > 1.0f ||
		min_link_score < 0.0f || min_link_score > 1.0f ||
		power_d_factor < 0.0f || power_d_factor > 1.0f ||
		importance_boundary < 0.0f || importance_boundary > 1.0f ||
		clusterization_sim_threshold < 0.0f || clusterization_sim_threshold > 1.0f ||
		max_hour_diff < 0.0f || top_k_value < 0 )
		return false;

	m_iDocCount = doc_count;
	m_fSoftOr = soft_or;
	m_fMinDocRank = min_doc_rank;
	m_fMinLinkScore = min_link_score;
	m_fPowerDFactor = power_d_factor;
	m_fDocumentImportanceBoundary = importance_boundary;
	m_bDocImportance = is_doc_importance;
	m_bTemporalMode = is_temporal;
	m_bIsW2V = is_w2v;
	m_bClusterization = is_clusterization;
	m_bPyramidFeature = is_pyramid;
	m_bLexRank = is_lexrank;
	m_fClusterizationSimThreshold = clusterization_sim_threshold;
	m_fMaxHourDiff = max_hour_diff;
	m_iTopKValue = top_k_value;

	return true;
}

bool TSDocumentExtractor::PrintTopDocs(const TSDocCollection &whole_collection, const std::map<std::string, float> doc_to_importance, const std::vector<std::string> top_docs) const
{
	std::fstream pFile;
	pFile.open("TopDoc.xml", std::ios::app);
	auto PrintDoc = [] (const TSDocument &doc) {
		
	};
}

bool TSDocumentExtractor::ConstructTimeLineCollections(const TSQuery &query, TSTimeLineCollections &collections)
{
	auto probe = CProfiler::CProfilerProbe("constr_timeline_collections");

	TSDocCollection whole_collection;
	auto init_params = { (float)m_iDocCount, m_fSoftOr, m_fMinDocRank };
	if( m_pDataExtractor->GetDocuments(query, init_params, whole_collection) != ReturnCode::TS_NO_ERROR )
		return false;

	if( whole_collection.size() == 0 )
		return false;

	if( m_bTemporalMode ) {
		if( m_bDocImportance ) {
			std::map<std::string, float> doc_to_importance;
			std::vector<std::string> top_docs;
			ComputeDocsImportance(whole_collection, doc_to_importance, top_docs);
			// Print top docs

			//
			collections.InitDocumentsImportanceData(std::move(doc_to_importance), std::move(top_docs));
			if( !SeparateCollectionByTime(whole_collection, collections) )
				return false;

		}
		else if( !SeparateCollectionByTime(whole_collection, collections) )
			return false;
	} else {
		while( whole_collection.size() > 0 ) {
			auto doc_iter = whole_collection.begin();
			collections.AddDocNode(whole_collection.ExtractNode(doc_iter->first), 0);
		}
	}

	return true;
}

bool TSDocumentExtractor::SeparateCollectionByTime(TSDocCollection &whole_collection, TSTimeLineCollections &collections) const
{
	auto probe = CProfiler::CProfilerProbe("separate_by_time");
	while( whole_collection.size() > 0 ) {
		auto doc_iter = whole_collection.begin();
		std::string s_int_date;
		if( !doc_iter->second.GetMetaData(SMetaDataType::INT_DATE, s_int_date) )
			return false;

		collections.AddDocNode(whole_collection.ExtractNode(doc_iter->first), std::stoi(s_int_date));
	}
	CLogger::Instance()->WriteToLog("INFO : collections days before cut = " + std::to_string(collections.size()));
	CutDaysWithSmallPublicationsSize(collections);
	CLogger::Instance()->WriteToLog("INFO : collections days after cut = " + std::to_string(collections.size()));

	return true;
}

void TSDocumentExtractor::CutDaysWithSmallPublicationsSize(TSTimeLineCollections &collections) const
{
	auto probe = CProfiler::CProfilerProbe("cut_collections");
	std::vector<int> coll_sizes;
	coll_sizes.reserve(collections.size());
	for( const auto &day_collection : collections ) {
		coll_sizes.push_back((int)day_collection.second.size());
	}

	std::sort(coll_sizes.begin(), coll_sizes.end(), std::greater<int>());
	int summ_top_3 = 0;
	for( int i = 0; i < std::min(3, (int)coll_sizes.size()); i++ )
		summ_top_3 += coll_sizes[i];

	summ_top_3 /= std::min(3, (int)coll_sizes.size());

	collections.EraseCollectionsWithSizeLessThen((int)(summ_top_3 * 0.2f));
}

void TSDocumentExtractor::ComputeDocsImportance(const TSDocCollection &whole_collection, std::map<std::string, float> &doc_to_importance, std::vector<std::string> &top_docs) const
{
	auto probe = CProfiler::CProfilerProbe("compute_doc_importance");
	std::vector<TSDocumentRepresentation> docs_representations;
	ConstructDocumentsRepresentations(whole_collection, docs_representations);

	if( docs_representations.empty() )
		return;

	bool is_reporting_graph = true;
	std::vector<std::vector<float>> report_similarity_matrix;

	std::vector<float> importances, start_importances;
	start_importances.resize(docs_representations.size());
	std::fill(start_importances.begin(), start_importances.end(), m_fPowerDFactor / docs_representations.size());
	//__debugbreak();
	if( m_bClusterization ) {
		std::vector<TSDocumentsCluster> clusters;
		std::vector<std::vector<float>> clusters_similarity_matrix;

		ClusterizationProcess(docs_representations, clusters, clusters_similarity_matrix);
		ComputeStartImportanceVector(clusters, (int)docs_representations.size(), start_importances);

		if( m_bPyramidFeature ) {
			std::vector<std::vector<float>> docs_pyramid_similarity_matrix, clusters_pyramid_similarity_matrix;
			ConstructSimilarityMatrix(docs_representations, docs_pyramid_similarity_matrix);
			ConstructClustersSimilarityMatrix(clusters, docs_representations, docs_pyramid_similarity_matrix, clusters_pyramid_similarity_matrix);
			PowerMethod(clusters_pyramid_similarity_matrix, start_importances, importances);
			if( is_reporting_graph )
				report_similarity_matrix = clusters_pyramid_similarity_matrix;
		} else {
			if( m_bLexRank )
				PowerMethod(clusters_similarity_matrix, start_importances, importances);
			else {
				importances = start_importances;
			}

			if( is_reporting_graph )
				report_similarity_matrix = clusters_similarity_matrix;
		}

		NormalizeImportanceVector(importances);
		for( int i = 0; i < importances.size(); i++ )
			clusters[i].SetImportance(importances[i]);

		std::sort(clusters.begin(), clusters.end(), [] (const TSDocumentsCluster &lhs, const TSDocumentsCluster &rhs) {
			return lhs.GetImportance() > rhs.GetImportance();
		});

		for( const auto doc_repr : docs_representations )
			doc_to_importance.insert(doc_repr.GetPair());

		for( const auto &cluster : clusters ) {
			if( cluster.GetImportance() > m_fDocumentImportanceBoundary ) {
				CLogger::Instance()->WriteToLog("INFO : topdoc = " + cluster.GetCentroidDoc()->GetDocPtr()->GetDocID() + " i = " + std::to_string(cluster.GetImportance()));
				top_docs.push_back(cluster.GetCentroidDoc()->GetDocPtr()->GetDocID());
			}
		}

		if( is_reporting_graph )
			TSDocumentExtractor::ReportGraph(report_similarity_matrix, importances, clusters);

	} else {
		std::vector<std::vector<float>> docs_pyramid_similarity_matrix;
		ConstructSimilarityMatrix(docs_representations, docs_pyramid_similarity_matrix);
		PowerMethod(docs_pyramid_similarity_matrix, start_importances, importances);
		NormalizeImportanceVector(importances);

		for( int i = 0; i < importances.size(); i++ ) {
			docs_representations[i].SetImportance(importances[i]);
			doc_to_importance.insert(docs_representations[i].GetPair());
		}

		std::sort(docs_representations.begin(), docs_representations.end(), [] (const TSDocumentRepresentation &lhs, const TSDocumentRepresentation &rhs) {
			return lhs.GetImportance() > rhs.GetImportance();
		});

		for( const auto &doc : docs_representations ) {
			if( doc.GetImportance() > m_fDocumentImportanceBoundary ) {
				CLogger::Instance()->WriteToLog("INFO : topdoc = " + doc.GetDocPtr()->GetDocID() + " i = " + std::to_string(doc.GetImportance()));
				top_docs.push_back(doc.GetDocPtr()->GetDocID());
			}
		}
	}
}

void TSDocumentExtractor::ConstructSimilarityMatrix(const std::vector<TSDocumentRepresentation> &docs_representations, std::vector<std::vector<float>> &similarity_matrix) const
{
	auto probe = CProfiler::CProfilerProbe("construct_sim_matrix");
	int size = (int)docs_representations.size();
	similarity_matrix.resize(size);
	for( auto &row : similarity_matrix ) {
		row.resize(size);
		std::fill(row.begin(), row.end(), 0.f);
	}

	for( int i = 0; i < size; i++ ) {
		for( int j = 0; j < size; j++ ) {
			if( j == i )
				continue;
			// if reference from j to i
			if( docs_representations[j].GetHourDate() > docs_representations[i].GetHourDate() ) {
				float sim = docs_representations[j] * docs_representations[i];
				if( sim > m_fMinLinkScore )
					similarity_matrix[i][j] = sim;
			}
		}
	}
}

void TSDocumentExtractor::PowerMethod(const std::vector<std::vector<float>> &similarity_matrix, const std::vector<float> &start_importances, std::vector<float> &importances) const
{
	//__debugbreak();
	auto probe = CProfiler::CProfilerProbe("power_method");
	int size = (int)similarity_matrix.size();
	importances.resize(size);

	std::vector<float> importance_next, importance_last;
	importance_last = importance_next = start_importances;

	std::vector<int> degrees(size, 0);
	for( int i = 0; i < size; i++ )
		for( int j = 0; j < size; j++ )
			if( similarity_matrix[i][j] > 0.f )
				degrees[j]++;
	

	float max_dif = 1.f, epsilon = 0.000001f;

	while( max_dif > epsilon ) {
		max_dif = 0;
		// calculate importance_next
		for( int i = 0; i < size; i++ ) {
			if( importance_last[i] < FLT_EPSILON )
				continue;

			importance_next[i] = m_fPowerDFactor / size;

			for( auto j = 0; j < size; j++ ) {
				if( similarity_matrix[i][j] > 0.f ) {
					importance_next[i] += (1.f - m_fPowerDFactor) * similarity_matrix[i][j] * importance_last[j] / degrees[j];
				}
			}

			float dif = std::fabs(importance_next[i] - importance_last[i]);
			max_dif = std::max(max_dif, dif);
			
		}
		importance_last.swap(importance_next);
	}

	importances.swap(importance_last);
}

void TSDocumentExtractor::ReportGraph(const std::vector<std::vector<float>> &similarity_matrix, const std::vector<float> &importances, const std::vector<TSDocumentsCluster> &clusters)
{
	std::string node_data;
	const int size = (int)clusters.size();
	for( int i = 0; i < size; i++ ) {
		std::string date = std::to_string(clusters[i].GetCentroidHourDate()),
			imp = std::to_string(importances[i]),
			label = clusters[i].GetCentroidDoc()->GetDocPtr()->GetDocID(),
			cluster_label = std::to_string(i);
		node_data += date + " " + imp + " " + label + " " +  cluster_label + "\n";
	}

	std::string edge_data;
	for( int i = 0; i < size; i++ ) {
		for( int j = 0; j < size; j++ ) {
			if( i == j || similarity_matrix[i][j] < DBL_EPSILON )
				continue;
			edge_data += std::to_string(j) + " " + std::to_string(i) + " " + std::to_string(similarity_matrix[i][j]) + "\n";
		}
	}

#pragma omp critical (ReportGraph) 
	{
		std::string node_file_name = "node_graph_num_" + std::to_string(m_iGraphsCount),
			edge_file_name = "edge_graph_num_" + std::to_string(m_iGraphsCount);

		std::fstream pNodeFile, pEdgeFile;
		pNodeFile.open(node_file_name, std::fstream::out);
		pEdgeFile.open(edge_file_name, std::fstream::out);
		if( pNodeFile.is_open() && pEdgeFile.is_open() ) {
			pNodeFile << node_data;
			pEdgeFile << edge_data;

			m_iGraphsCount++;
			pNodeFile.close();
			pEdgeFile.close();
		}
	}

	// print centroid info
#pragma omp critical (ReportClusters) 
	{
		CLogger::Instance()->WriteToLog("Report clusters");
		for( const auto &cluster : clusters ) {
			if( !cluster.IsActive() )
				continue;
			CLogger::Instance()->WriteToLog("Cluster ID=" + std::to_string(cluster.GetClusterLabel()));
			CLogger::Instance()->WriteToLog("\tCluster CentroidDoc=" + cluster.GetCentroidDoc()->GetDocPtr()->GetDocID());
			for( int i = 0; i < std::min(10, (int)cluster.GetClusterCentroid().size()); i++ ) {
				const auto &item = *(cluster.GetClusterCentroid().begin() + i);
				CLogger::Instance()->WriteToLog("\tTerm=" + (std::string)item.GetID() + " Weight=" + std::to_string(item.GetWeight()));
			}
		}
	}
}
void TSDocumentExtractor::NormalizeImportanceVector(std::vector<float> &importances) const
{
	float max_value = *std::max_element(importances.begin(), importances.end()),
		min_value = 1.;
	for( int i = 0; i < importances.size(); i++ ) {
		if( importances[i] < FLT_EPSILON )
			continue;
		if( importances[i] < min_value )
			min_value = importances[i];
	}

	float diff = max_value - min_value;
	if( max_value > m_fPowerDFactor / importances.size() && diff > 0 ) {
		for( auto &elem : importances )
			if( elem > FLT_EPSILON )
				elem = (elem - min_value) / diff;
	}
}

float TSDocumentExtractor::CalculateHourSim(float h1, float h2) const
{
	float sim = std::max(0.f, 1.f - fabs(h1 - h2) / m_fMaxHourDiff);
	float log_sim = std::log(1.f + sim) / std::log(2.f);
	return log_sim;
}

void TSDocumentExtractor::ClusterizationProcess(std::vector<TSDocumentRepresentation> &docs_representations, std::vector<TSDocumentsCluster> &clusters, std::vector<std::vector<float>> &clusters_sim_matrix) const
{
	auto probe = CProfiler::CProfilerProbe("clusterization");
	const size_t size = docs_representations.size();
	clusters_sim_matrix.resize(size);
	for( int i = 0; i < size; i++ ) {
		clusters_sim_matrix[i].resize(size);
		std::fill(clusters_sim_matrix[i].begin(), clusters_sim_matrix[i].end(), 0.f);
	}

	clusters.reserve(docs_representations.size());
	for( auto &doc_repr : docs_representations )
		clusters.emplace_back(&doc_repr);

	//__debugbreak();
	float max_simm = 0;
	for( int i = 0; i < size; i++ ) {
		for( int j = 0; j < i; j++ ) {
			float hour_sim = CalculateHourSim((float)clusters[i].GetCentroidHourDate(), (float)clusters[j].GetCentroidHourDate());
			if( hour_sim < FLT_EPSILON )
				continue;
			float sim = clusters[i].GetClusterCentroid() * clusters[j].GetClusterCentroid() * hour_sim;
			clusters_sim_matrix[i][j] = sim;
			clusters_sim_matrix[j][i] = sim;
			max_simm = std::max(max_simm, sim);
		}
	}

	float max_sim = 1.f;
	bool some_activity = true;
	while( some_activity && max_sim > m_fClusterizationSimThreshold ) {
		max_sim = 0.f;
		some_activity = false;
		std::pair<int, int> max_sim_pair;
		for( int i = 0; i < size; i++ ) {
			if( !clusters[i].IsActive() )
				continue;

			for( int j = 0; j < i; j++ ) {
				if( !clusters[j].IsActive() )
					continue;

				float curr_sim = clusters_sim_matrix[i][j];
				if( curr_sim > max_sim ) {
					max_sim = curr_sim;
					max_sim_pair.first = i;
					max_sim_pair.second = j;
				}
				some_activity = true;
			}
		}
		if( max_sim > m_fClusterizationSimThreshold ) {
			clusters[max_sim_pair.first].MergeClusters(clusters[max_sim_pair.second]);

			for( int i = 0; i < size; i++ ) {
				float sim = 0.f;
				float hour_sim = CalculateHourSim((float)clusters[i].GetCentroidHourDate(), (float)clusters[max_sim_pair.first].GetCentroidHourDate());
				if( clusters[i].IsActive() && i != max_sim_pair.first && hour_sim >= FLT_EPSILON )
					sim = clusters[max_sim_pair.first].GetClusterCentroid() * clusters[i].GetClusterCentroid() * hour_sim;

				clusters_sim_matrix[i][max_sim_pair.first] = sim;
				clusters_sim_matrix[max_sim_pair.first][i] = sim;

				clusters_sim_matrix[i][max_sim_pair.second] = 0.f;
				clusters_sim_matrix[max_sim_pair.second][i] = 0.f;
			}
		}
	}

	for( auto &cluster : clusters ) {
		std::sort(cluster.GetClusterCentroid().begin(), cluster.GetClusterCentroid().end(), [] (const TSIndexItem &lhs, const TSIndexItem &rhs) {
			return lhs.GetWeight() > rhs.GetWeight();
		});
	}
}

void TSDocumentExtractor::ComputeStartImportanceVector(const std::vector<TSDocumentsCluster> &clusters, int docs_size, std::vector<float> &start_importances) const
{
	for( int i = 0; i < clusters.size(); i++ ) {
		if( clusters[i].size() == 0 )
			start_importances[i] = 0.f;

		start_importances[i] *= std::log(M_E + (float)clusters[i].size());
	}
}

void TSDocumentExtractor::ConstructClustersSimilarityMatrix(const std::vector<TSDocumentsCluster> &clusters, const std::vector<TSDocumentRepresentation> &docs_representations, const std::vector<std::vector<float>> &similarity_matrix, std::vector<std::vector<float>> &clusters_similarity_matrix) const
{
	const size_t cluster_size = clusters.size();
	clusters_similarity_matrix.resize(cluster_size);
	for( int i = 0; i < cluster_size; i++ ) {
		clusters_similarity_matrix[i].resize(cluster_size);
		std::fill(clusters_similarity_matrix[i].begin(), clusters_similarity_matrix[i].end(), 0.f);
	}

	const size_t doc_size = docs_representations.size();
	for( int i = 0; i < doc_size; i++ ) {
		for( int j = 0; j < doc_size; j++ ) {
			const float sim = similarity_matrix[i][j];
			if( sim > 0.f ) {
				int i_cluster_id = docs_representations[i].GetClusterLabel(),
					j_cluster_id = docs_representations[j].GetClusterLabel();
				if( (j_cluster_id != i_cluster_id) && clusters[i_cluster_id].IsActive() && clusters[j_cluster_id].IsActive() )
					clusters_similarity_matrix[i_cluster_id][j_cluster_id] += sim;
			}
		}
	}

	for( int i = 0; i < cluster_size; i++ ) {
		float sum = std::accumulate(clusters_similarity_matrix[i].begin(), clusters_similarity_matrix[i].end(), 0.f);
		if( sum > 0.f ) {
			std::for_each(clusters_similarity_matrix[i].begin(), clusters_similarity_matrix[i].end(), [sum] (float &sim) { sim /= sum; });
		}
	}
}

void TSDocumentExtractor::ConstructDocumentsRepresentations(const TSDocCollection &whole_collection, std::vector<TSDocumentRepresentation> &docs_representations) const
{
	docs_representations.reserve(whole_collection.size());
	int i = 0;
	for( const auto &doc_pair : whole_collection ) {
		docs_representations.emplace_back(i);
		ConstructDocRepresentation(doc_pair.second, docs_representations[i]);
		i++;
	}
}

void TSDocumentExtractor::ConstructDocRepresentation(const TSDocument &document, TSDocumentRepresentation &doc_representation) const
{
	doc_representation.InitDocPtr(&document);
	doc_representation.InitIsW2V(m_bIsW2V);

	int i = 0;
	for( auto sentences_iter = document.sentences_begin(); sentences_iter != document.sentences_end() && i < m_iDocHeadSize; sentences_iter++, i++ ) {
		TSSentenceConstPtr sentence_ptr = &(*sentences_iter);
		if( m_bIsW2V ) {
			TSIndexConstPtr index_ptr;
			if( !sentence_ptr->GetIndex(SDataType::LEMMA, index_ptr) || !index_ptr->ConstructIndexEmbedding(m_pModel) )
				continue;
		}
		doc_representation.AddHead(sentence_ptr);
	}

	i = 0;
	for( auto sentences_iter = document.sentences_rbegin(); sentences_iter != document.sentences_rend() && i < m_iDocTailSize; sentences_iter++, i++ ) {
		TSSentenceConstPtr sentence_ptr = &(*sentences_iter);
		if( IsSentenceHasReferenceToThePast(sentence_ptr) ) {
			if( m_bIsW2V ) {
				TSIndexConstPtr index_ptr;
				if( !sentence_ptr->GetIndex(SDataType::LEMMA, index_ptr) || !index_ptr->ConstructIndexEmbedding(m_pModel) )
					continue;
			}
			doc_representation.AddTail(sentence_ptr);
		}
	}
	
	//__debugbreak();
	// constuct topK
	using queue_type = std::pair<float, int>;
	std::priority_queue<queue_type, std::vector<queue_type>, std::greater<queue_type>> top_k;
	auto CreateQueue = [this] (std::priority_queue<queue_type, std::vector<queue_type>, std::greater<queue_type>> &queue_top, const TSIndexConstPtr &index) {
		for( int i = 0; i < index->size(); i++ ) {
			const auto &index_item_iter = (index->begin() + i);
			if( queue_top.size() < m_iTopKValue ) {
				queue_top.push(std::make_pair(index_item_iter->GetWeight(), i));
			}
			else if( index_item_iter->GetWeight() > queue_top.top().first ) {
				queue_top.pop();
				queue_top.push(std::make_pair(index_item_iter->GetWeight(), i));
			}
		}
	};

	constexpr SDataType index_type = SDataType::LEMMA;
	TSIndexConstPtr const_index_ptr;
	if( document.GetIndex(index_type, const_index_ptr) ) {
		CreateQueue(top_k, const_index_ptr);
		for( const auto &elem : GetPrivateContainer<queue_type, std::vector<queue_type>, std::greater<queue_type>>(top_k) )
			doc_representation.GetTopKTerms().AddToIndex(*(const_index_ptr->begin() + elem.second));

		std::sort(doc_representation.GetTopKTerms().begin(), doc_representation.GetTopKTerms().end());
	}
}

bool TSDocumentExtractor::IsSentenceHasReferenceToThePast(TSSentenceConstPtr sentence) const
{
	TSIndexConstPtr p_index;
	if( !sentence->GetIndex(SDataType::LEMMA, p_index) )
		return false;

	if( p_index->size() < 4 /*m_iMinSentenceSize*/ || p_index->size() > 30 /*m_MaxSentenceSize*/ )
		return false;

	return sentence->GetDocPtr()->sentences_size() - sentence->GetSentenseNumber() <= m_iDocTailSize;
}

bool TSDocumentRepresentation::InitDocPtr(TSDocumentConstPtr doc_ptr)
{
	if( !doc_ptr )
		return false;

	m_pDoc = doc_ptr;
	if( !m_pDoc->GetMetaData(SMetaDataType::DATE, m_sDate) )
		return false;

	auto int_date_by_hour = [] (const std::string &data) {
		std::string day = data.substr(0, 2);
		std::string month = data.substr(3, 2);
		std::string year = data.substr(6, 4);
		std::string hour = data.substr(11, 2);
		if( !utils::IsStringIntNumber(day) || !utils::IsStringIntNumber(month) || !utils::IsStringIntNumber(year) || !utils::IsStringIntNumber(hour) )
			return -1;

		return std::stoi(hour) + std::stoi(day) * 24 + std::stoi(month) * 24 * 31 + std::stoi(year) * 24 * 365;
	};
	m_iHourDate = int_date_by_hour(m_sDate);

	return true;
}

void TSDocumentRepresentation::AddHead(TSSentenceConstPtr sentence_ptr)
{
	m_HeadSentences.push_back(sentence_ptr);
}

void TSDocumentRepresentation::AddTail(TSSentenceConstPtr sentence_ptr)
{
	m_TailSentences.push_back(sentence_ptr);
}

float TSDocumentRepresentation::operator*(const TSDocumentRepresentation &other) const
{
	float weight = 0.f;
	for( auto tail_sentence : m_TailSentences ) {
		for( auto head_sentence : other.m_HeadSentences ) {
			if( m_bIsW2V ) {
				float w = tail_sentence->EmbeddingSimilarity(*head_sentence, SDataType::LEMMA);
				weight = std::max(weight, w);
			} else {
				float w = *tail_sentence * *head_sentence;
				weight = std::max(weight, w);
			}
		}
	}
	return weight;
}

TSDocumentsCluster::TSDocumentsCluster(TSDocumentRepresentation *init_doc_repr) :
	m_Centroid(SDataType::LEMMA),
	m_bIsActive(true),
	m_iClusterLabel(init_doc_repr->GetClusterLabel()),
	m_fImportance(0.f)
{
	m_Docs.emplace_back(init_doc_repr);
	AddDocDataToClusterInfo(m_Docs.back());
	ComputeCentroid();
}

void TSDocumentsCluster::MergeClusters(TSDocumentsCluster &other_cluster)
{
	for( auto &doc : other_cluster.m_Docs ) {
		doc->InitClusterLabel(m_iClusterLabel);
		AddDocDataToClusterInfo(doc);
	}
	other_cluster.m_bIsActive = false;

	m_Docs.splice(m_Docs.end(), other_cluster.m_Docs);
	ComputeCentroid();
}

float TSDocumentsCluster::Sim(const TSDocumentsCluster &other, const std::vector<std::vector<float>> &sim_matrix) const
{
	float min_sim = 1.f;
	for( const auto &doc : m_Docs )
		for( const auto &other_doc : other.m_Docs )
			min_sim = std::min(min_sim, sim_matrix[doc->GetID()][other_doc->GetID()]);
	return min_sim;
}

void TSDocumentsCluster::SetImportance(float imp)
{
	for( auto &doc : m_Docs )
		doc->SetImportance(imp);
	m_fImportance = imp;
}

void TSDocumentsCluster::AddDocDataToClusterInfo(TSDocumentRepresentation* doc)
{
	for( const auto &item : doc->GetTopKTerms() ) {
		auto iter = m_CentroidInfo.find(item.GetID());
		if( iter == m_CentroidInfo.end() ) {
			m_CentroidInfo[item.GetID()] = item.GetWeight();
		} else {
			iter->second += item.GetWeight();
		}
	}
}

void TSDocumentsCluster::ComputeCentroid()
{
	if( !m_bIsActive || m_Docs.empty() )
		return;

	// fill centroid
	m_Centroid.erase(m_Centroid.begin(), m_Centroid.end());
	m_Centroid.reserve(m_CentroidInfo.size());
	for( const auto &centroid_item : m_CentroidInfo )
		m_Centroid.AddToIndex(TSIndexItem(centroid_item.first, centroid_item.second / m_Docs.size(), std::vector<int>()));

	// sort centroid
	/*std::sort(m_Centroid.begin(), m_Centroid.end(), [] (const TSIndexItem &lhs, const TSIndexItem &rhs) {
		return lhs.GetWeight() > rhs.GetWeight();
	});*/
	std::sort(m_Centroid.begin(), m_Centroid.end());

	m_iCentroidHourDate = 0;
	/*for( const auto &doc : m_Docs )
		m_iCentroidHourDate += doc->GetHourDate();
	m_iCentroidHourDate /= (int)m_Docs.size();*/

	ComputeCentroidDoc();
	m_iCentroidHourDate = m_pCentroidDoc->GetHourDate();
}

void TSDocumentsCluster::ComputeCentroidDoc()
{
	float max_sim = 0;
	for( const auto &doc : m_Docs ) {
		float curr_sim = doc->GetTopKTerms() * m_Centroid;
		if( curr_sim > max_sim ) {
			m_pCentroidDoc = doc;
			max_sim = curr_sim;
		}
	}
}