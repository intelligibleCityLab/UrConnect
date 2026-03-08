#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/directed_graph.hpp>

struct VertexProperties
{
	std::string VertexStringProperty;
};

//typedef boost::property<boost::edge_weight_t, double> EdgeWeightProperty;
//typedef boost::property<boost::edge_index_t, int> EdgeIndexProperty;

typedef boost::property<boost::edge_index_t, int, boost::property<boost::edge_weight_t, double >> EdgeProperty;
typedef boost::adjacency_list < boost::listS, boost::vecS, boost::undirectedS, VertexProperties, EdgeProperty> Graph;
typedef boost::adjacency_list < boost::listS, boost::vecS, boost::bidirectionalS, VertexProperties, EdgeProperty> Graph_d;

typedef boost::graph_traits < Graph >::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits < Graph >::edge_descriptor edge_descriptor;

typedef boost::graph_traits < Graph >::in_edge_iterator in_edge_iterator;
typedef boost::graph_traits < Graph >::out_edge_iterator out_edge_iterator;

typedef boost::graph_traits < Graph >::edge_iterator edge_iterator;

typedef std::pair<int, int> Edge;
