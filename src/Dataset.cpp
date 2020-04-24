#include <Dataset.hpp>
#include <utils.hpp>

#include <thrust/system/cuda/experimental/pinned_allocator.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>

Dataset::Dataset(std::string data_path, bool shuffle): shuffle(shuffle), train_data_index(0), test_data_index(0) {
    this->read_images(data_path + "/train-images-idx3-ubyte", this->train_data);
    this->read_labels(data_path + "/train-labels-idx1-ubyte", this->train_label);

    this->read_images(data_path + "/t10k-images-idx3-ubyte", this->test_data);
    this->read_labels(mnist_data_path + "/t10k-labels-idx1-ubyte", this->test_label);

}

void Dataset::reset() {
    this->train_data_index = 0;
    this->test_data_index = 0;

    if(shuffle) {
        unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count() % 1234;
        std::shuffle(this->train_data.begin(), this->train_data.end(), std::default_random_engine(seed));
        std::shuffle(this->train_label.begin(), this->train_label.end(), std::default_random_engine(seed));
    }
}

bool Dataset::has_next(bool is_train) {
    if(is_train) {
        return this->train_data_index < this->train_data.size();
    } else {
        return this->test_data_index < this->test_data.size();
    }
}

unsigned int Dataset::reverse_int(unsigned int i) {
    unsigned char ch1, ch2, ch3, ch4;
    ch1 = i & 255; //get last 8 bits
    ch2 = (i >> 8) & 255;
    ch3 = (i >> 16) & 255;
    ch4 = (i >> 24) & 255;
    return ((unsigned int)ch1 << 24) + ((unsigned int)ch2 << 16) + ((unsigned int)ch3 << 8) + ch4;

}

void Dataset::read_images(std::string file_name, std::vector<std::vector<float>>& output) {
    std::ifstream file(file_name, std::ios::binary);
    if(file.is_open()) {
        unsigned int magic_number = 0;
        unsigned int num_images = 0;
        unsigned int num_rows = 0;
        unsigned int num_cols = 0;

        file.read((char*)&magic_number, sizeof(magic_number));
        file.read((char*)&num_images, sizeof(num_images));
        file.read((char*)&num_rows, sizeof(num_rows));
        file.read((char*)&num_cols, sizeof(num_cols));

        magic_number = this->reverse_int(magic_number);
        number_of_images = this->reverse_int(number_of_images);
        n_rows = this->reverse_int(n_rows);
        n_cols = this->reverse_int(n_cols);

        std::cout << file_name << std::endl;
        std::cout << "magic number = " << magic_number << std::endl;
        std::cout << "number of images = " << number_of_images << std::endl;
        std::cout << "rows = " << n_rows << std::endl;
        std::cout << "cols = " << n_cols << std::endl;

        this->height = n_rows;
        this->width = n_cols;

        std::vector<unsigned char> image(n_rows * n_cols);
        std::vector<float> normalized_image(n_rows * n_cols);

        for (int i = 0; i < number_of_images; i++) {
            file.read((char*)&image[0], sizeof(unsigned char) * n_rows * n_cols);

            for (int i = 0; i < n_rows * n_cols; i++) {
                normalized_image[i] = (float)image[i] / 255 - 0.5;
            }
            output.push_back(normalized_image);
        }

    }
}

void DataSet::read_labels(std::string file_name, std::vector<unsigned char>& output) {
    std::ifstream file(file_name, std::ios::binary);
    if (file.is_open()) {
        unsigned int magic_number = 0;
        unsigned int number_of_images = 0;
        file.read((char*)&magic_number, sizeof(magic_number));
        file.read((char*)&number_of_images, sizeof(number_of_images));

        std::cout << file_name << std::endl;
        magic_number = this->reverse_int(magic_number);
        number_of_images = this->reverse_int(number_of_images);
        std::cout << "magic number = " << magic_number << std::endl;
        std::cout << "number of images = " << number_of_images << std::endl;

        for (int i = 0; i < number_of_images; i++) {
        unsigned char label = 0;
        file.read((char*)&label, sizeof(label));
        output.push_back(label);
        }
    }
}

void Dataset::forward(int batch_size, bool is_train) {
    if (is_train) {
        int start = this->train_data_index;
        int end = std::min(this->train_data_index + batch_size, (int)this->train_data.size());
        this->train_data_index = end;
        int size = end - start;

        std::vector<int> output_shape{size, 1, this->height, this->width}; //batch_size * 1 * im_height * im_width
        std::vector<int> output_label_shape{size, 10}; // batch_size * num_labels in dataset
        INIT_STORAGE(this->output, output_shape); //Initialize a Container to store the images
        INIT_STORAGE(this->output_label, output_labels_shape);
        thrust::fill(this->output_label->get_data().begin(), this->output_label->get_data.end(), 0); //set output labels to 0

        int image_stride = 1 * this->height * this->width;
        int label_stride = 10;

        thrust::host_vector<float, thrust::system::cuda::experimental::pinned_allocator<float>>train_data_buffer;
        train_data_buffer.reserve(size, image_stride);

        for(int i = start, i < end, i++) {
            train_data_buffer.insert(train_data_buffer.end(), this->train_data[i].begin(), this->train_data[i].end()); //insert data into container
            this->output_label->get_data()[(i - start) * one_hot_stride + this->train_label[i]] = 1; 
        }
        this->output->get_data() = train_data_buffer;
    } else {
        int start = this->test_data_index;
        int end = std::min(this->test_data_index + batch_size, (int)this->test-data.size());
        this->test_data_index = end;
        int size = end - start;

        std::vector<int> output_shape{size, 1, this->height, this->width}; //batch_size * 1 * im_height * im_width
        std::vector<int> output_label_shape{size, 10}; // batch_size * num_labels in dataset
        INIT_STORAGE(this->output, output_shape); //Initialize a Container to store the images
        INIT_STORAGE(this->output_label, output_labels_shape);
        thrust::fill(this->output_label->get_data().begin(), this->output_label->get_data.end(), 0); //set output labels to 0

        int image_stride = 1 * this->height * this->width;
        int label_stride = 10;

        thrust::host_vector<float, thrust::system::cuda::experimental::pinned_allocator<float>>test_data_buffer;
        test_data_buffer.reserve(size, image_stride);

        for(int i = start, i < end, i++) {
            test_data_buffer.insert(test_data_buffer.end(), this->test_data[i].begin(), this->test_data[i].end()); //insert data into container
            this->output_label->get_data()[(i - start) * one_hot_stride + this->test_label[i]] = 1; 
        }
        this->output->get_data() = test_data_buffer;
    }
}