//
//  main.cpp
//  flow_graph
//
//  Created by Ira on 10/12/16.
//  Copyright (c) 2016 Ira. All rights reserved.
//

#include <iostream>
#include <vector>
#include <fstream>
#include "tbb/flow_graph.h"

using namespace std;
using namespace tbb::flow;

const int max_value = 255;
const int n = 256; // number of rows
const int m = 256; // number of columns

struct image
{
    image()
    :height_(n)
    ,width_(m)
    ,id_(-1)
    {
        intensity_ = vector<vector<int>>(height_, vector<int>(width_));
    }
    
    image(int i)
    :height_(n)
    ,width_(m)
    ,id_(i)
    {
        intensity_ = vector<vector<int>>(height_, vector<int>(width_));
    }
    
    void fill()
    {
        for (int i = 0; i < height_; ++i)
            for (int j = 0; j < width_; ++ j)
                intensity_[i][j] = rand() % (max_value + 1);
    }
    
    pair<vector<pair<int, int>>, int> find_positions(int value) const
    {
        vector<pair<int, int>> res;
        for (int i = 0; i < height_; ++i)
            for (int j = 0; j < width_; ++ j)
                if (intensity_[i][j] == value)
                    res.push_back(make_pair(i,j));
        return make_pair(res, id_);
    }
    
    pair<vector<pair<int, int>>, int> find_mins() const
    {
        int current_min = max_value + 1;
        for (int i = 0; i < height_; ++i)
            for (int j = 0; j < width_; ++ j)
                if (intensity_[i][j] < current_min)
                    current_min = intensity_[i][j];
        return find_positions(current_min);
    }
    
    pair<vector<pair<int, int>>, int> find_maxs() const
    {
        int current_max = -1;
        for (int i = 0; i < height_; ++i)
            for (int j = 0; j < width_; ++ j)
                if (intensity_[i][j] > current_max)
                    current_max = intensity_[i][j];
        return find_positions(current_max);
    }
    
    void highlight_points(pair<vector<pair<int, int>>, int> inp)
    {
        vector<pair<int, int>> targets = inp.first;
        for (int i = 0; i < targets.size(); ++i)
            higlight_point(targets[i]);
    }
    
    float mean_intensity() const
    {
        int total_size = height_ * width_;
        float res = 0.f;
        for (int i = 0; i < height_; ++i)
            for (int j = 0; j < width_; ++ j)
                res += intensity_[i][j];
        return res / total_size;
        
    }
    
    void invert_intensity()
    {
        for (int i = 0; i < height_; ++i)
            for (int j = 0; j < width_; ++ j)
                intensity_[i][j] = max_value - intensity_[i][j];
    }
    
    int get_id() const
    {
        return id_;
    }
    
private:
    int height_;
    int width_;
    int id_;
    vector<vector<int>> intensity_;
    
    void higlight_point(pair<int, int> target)
    {
        int i = target.first;
        int j = target.second;
        for (int l = i-1; l <= i+1; ++l)
            if (l >= 0 && l < height_)
                for (int k = j-1; k <= j+1; ++k)
                    if (k >= 0 && k < width_)
                        intensity_[l][k] = 0;
    }
    
};

int find_option(string option, int argc, const char * argv[])
{
    for (int i = 1; i < argc; ++i)
        if (argv[i] == option)
            return i+1;
    return -1;
}

int get_image_id(image im)
{
    return im.get_id();
}

int get_pair_id(pair<vector<pair<int, int>>, int> p)
{
    return p.second;
}

int identity(int i)
{
    return i;
}

int main(int argc, const char * argv[])
{
    int max_images = stoul(argv[find_option("-l", argc, argv)]);
    int target_value = stoul(argv[find_option("-b", argc, argv)]);
    bool to_write = false;
    int log_i = find_option("-f", argc, argv);
    string log_file_name = "";
    if (log_i != -1)
    {
        log_file_name = argv[log_i];
        to_write = true;
    }
    
    cout << "Max # images: " << max_images << endl;
    cout << "Intesity value to look for: " << target_value << endl;
    if (to_write)
    {
        cout << "Output means: true" << endl;
        cout << "Output file: " << log_file_name << endl;
    }
    else
    {
        cout << "Output means: false" << endl;
    }
    
    graph g;
    int current_images_n = 0;
    int total_images_n = max_images + 3;
    ofstream log;
    if (to_write)
        log = ofstream(log_file_name);
    
    // 1. Source node
    source_node<image> source(g, [&](image& result){
        if (current_images_n >= total_images_n)
        {
            cout << "# images generated: " << current_images_n << endl;
            return false;
        }
        
        result = image(current_images_n);
        result.fill();
        
        current_images_n += 1;
        return true;
    }, false);

    // 2. Limiter node
    limiter_node<image> limit(g, max_images);
    
    // 3a. Min search
    function_node<image, pair<vector<pair<int, int>>, int>> search_min(g, unlimited, [](const image &inp_image) {
        return inp_image.find_mins();
    });
    
    // 3b. Max search
    function_node<image, pair<vector<pair<int, int>>, int>> search_max(g, unlimited, [](const image &inp_image) {
        return inp_image.find_maxs();
    });
    
    // 3c. Target search
    function_node<image, pair<vector<pair<int, int>>, int>> search_target(g, unlimited, [target_value](const image &inp_image) {
        return inp_image.find_positions(target_value);
    });
    
    // 4. Join node
    join_node<tuple<image, pair<vector<pair<int, int>>, int>, pair<vector<pair<int, int>>, int>, pair<vector<pair<int, int>>, int>>, key_matching<int>> join(g, get_image_id, get_pair_id, get_pair_id, get_pair_id);
    
    // 5. Highlight found positions
    function_node<tuple<image, pair<vector<pair<int, int>>, int>, pair<vector<pair<int, int>>, int>, pair<vector<pair<int, int>>, int>>, image> highlight(g, unlimited, [](tuple<image, pair<vector<pair<int, int>>, int>, pair<vector<pair<int, int>>, int>, pair<vector<pair<int, int>>, int>> input_data) {
        image current_image = get<0>(input_data);
        current_image.highlight_points(get<1>(input_data));
        current_image.highlight_points(get<2>(input_data));
        current_image.highlight_points(get<3>(input_data));
        return current_image;
    });
    
    // 6a. Invert image
    function_node<image, int> invert(g, unlimited, [](image inp_image) {
        inp_image.invert_intensity();
        return inp_image.get_id();
    });
    
    // 6b.1 Mean intensity
    function_node<image, pair<float, int>> mean_intensity(g, unlimited, [](const image& inp_image) {
        return make_pair(inp_image.mean_intensity(), inp_image.get_id());
    });
    
    // 6b.2 Write output
    function_node<pair<float, int>, int> write_log(g, unlimited, [&](pair<float, int> value){
        if (to_write)
            log << value.first << endl;
        return value.second;
    });
    
    // 7. Final join
    join_node<tuple<int, int>, key_matching<int>> final_join(g, identity, identity);
    
    // 8. Message limiter
    function_node<tuple<int, int>, continue_msg> message(g, unlimited, [](const tuple<int, int>& input_data) {
        return continue_msg();
    });

    
    make_edge(source, limit);
    make_edge(limit, search_min);
    make_edge(limit, search_max);
    make_edge(limit, search_target);
    make_edge(limit, input_port<0>(join));
    make_edge(search_min, input_port<1>(join));
    make_edge(search_max, input_port<2>(join));
    make_edge(search_target, input_port<3>(join));
    make_edge(join, highlight);
    make_edge(highlight, invert);
    make_edge(highlight, mean_intensity);
    make_edge(mean_intensity, write_log);
    make_edge(invert, input_port<0>(final_join));
    make_edge(write_log, input_port<1>(final_join));
    make_edge(final_join, message);
    make_edge(message, limit.decrement);

    cout << endl;
    source.activate();
    g.wait_for_all();
    if (to_write)
        log.close();
    
    return 0;
}
