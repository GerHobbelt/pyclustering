/**
*
* Copyright (C) 2014-2016    Aleksey Kukushkin (pyclustering@yandex.ru)
*
* GNU_PUBLIC_LICENSE
*   pyclustering is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   pyclustering is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include <vector>

#include "cluster/ant_clustering_params.hpp"

namespace ant {


/***********************************************************************
* ant_clustering_result - 
*                Result of clustering
*
*************************************************************************/
class ant_clustering_result
{
public:
    std::vector<std::vector<bool>> clusters;
};

class clustering_data
{
public:
    clustering_data(std::size_t count_data, std::size_t dimension)
        :data(count_data, std::vector<double>(dimension, 0.0))
    {}

    std::vector<std::vector<double>> data;
};


/***********************************************************************
* ant_clustering_mean
* 
*
*************************************************************************/
class ant_clustering_mean
{
public:

    ant_clustering_mean(std::shared_ptr<ant_clustering_params>& param_init)
        : result(new ant_clustering_result())
        , params{ param_init }
    {}

    std::shared_ptr<ant_clustering_result> process(const clustering_data& input, std::size_t count_clusters);


private:

    using AntAPI = ant::ant_colony_clustering_params_initializer;

    /*
    *    Wrappers to get parameters value
    */
    const AntAPI::base_param_type<params_name_clustering::RO>
        get_ro()const { return params->get<params_name_clustering::RO>().get(); }

    const AntAPI::base_param_type<params_name_clustering::PHERAMONE_INIT>
        get_pheramone_init()const { return params->get<params_name_clustering::PHERAMONE_INIT>().get(); }

    const AntAPI::base_param_type<params_name_clustering::ITERATIONS>
        get_iterations()const { return params->get<params_name_clustering::ITERATIONS>().get(); }

    const AntAPI::base_param_type<params_name_clustering::COUNT_ANTS>
        get_count_ants()const { return params->get<params_name_clustering::COUNT_ANTS>().get(); }


    /*
    *   Pheramone
    */
    class pheramone
    {
    public:
        pheramone(std::size_t size_of_data, std::size_t count_clusters, double initialPheramone)
            : value(size_of_data, std::vector<double>(count_clusters, initialPheramone))
        {}

        const std::vector<double>& operator[] (std::size_t idx) const { return value[idx]; }
        std::vector<double>& operator[] (std::size_t idx) { return value[idx]; }

    private:
        std::vector<std::vector<double>> value;
    };


    /*
    *   A agent (ant) for clustering problem
    */
    class Ant
    {
    public:
        Ant(std::size_t count_data, std::size_t count_clusters)
            : clustering_data(count_data, std::vector<bool>(count_clusters, false))
        {}

        void clear()
        {
            for (std::vector<bool>& cluster : clustering_data)
                for(std::size_t i = 0; i < cluster.size(); ++i)
                    cluster[i] = false;
        }

        std::vector<std::vector<bool>> clustering_data;
        double F;
    };


    /*
    *        Algorithms sub functions
    */
    void clustering_by_pheramone(const pheramone& ph, const clustering_data& input, std::vector<Ant>& ants);

    unsigned get_random_number(unsigned max)    const { return (std::rand() % max); }
    std::size_t realize_pheromone(const pheramone& ph, std::size_t data_num);

    void calculate_F(std::vector<Ant>& ants, const clustering_data& input, std::size_t count_clusters, std::size_t dimension);
    void calculate_cluster_centers(const clustering_data& input
                                    , Ant& ant
                                    , std::vector<std::vector<double>>& cluster_centers
                                    , std::size_t count_clusters
                                    , std::size_t dimension                );

    void update_pheramone(pheramone& ph, const std::vector<Ant>& ants, std::size_t size_of_data, std::size_t count_clusters);

    void update_best_clustering(const std::vector<Ant>& ants, std::vector<std::vector<bool>>& best_clustering);

private:
    std::shared_ptr<ant_clustering_result> result;
    std::shared_ptr<ant_clustering_params> params;
};





}//namespace ant