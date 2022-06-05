#include "misc.h"

namespace misc {
	std::vector<std::string> split(std::string& s, std::string delim) {
		std::vector<std::string> command_parts;
		if(s.size() == 0)
			return command_parts;
		size_t start;
		size_t end = 0;
		while ((start = s.find_first_not_of(delim, end)) != std::string::npos)
		{
			end = s.find(delim, start);
			auto test = s.substr(start, end - start);
			command_parts.push_back(test);
		}
		return command_parts;
	}
	
	std::vector<std::string> splitCommand(std::string command) {
		return split(command, " ");
	}
	
	
	std::string comGet(std::vector<std::string> command_parts, unsigned int part_number) {
		if(command_parts.size() <= part_number)
			return "";
		else
			return command_parts[part_number];
	}
	
	unsigned long long nanoNow() {
		return std::chrono::nanoseconds(
				std::chrono::high_resolution_clock::now().time_since_epoch()
				).count();
	}
	
	std::string url_encode(std::string& s) {
		CURL* c = curl_easy_init();
		char* out_raw = curl_easy_escape(c, s.c_str(), s.size());
		std::string out(out_raw);
		curl_free(out_raw);
		curl_easy_cleanup(c);
		return out;
	}
	
	std::string url_decode(std::string& s) {
		CURL* c = curl_easy_init();
		char* out_raw = curl_easy_unescape(c, s.c_str(), s.size(), NULL);
		std::string out(out_raw);
		curl_free(out_raw);
		curl_easy_cleanup(c);
		return out;
	}
	
	static size_t download_write(void *data, size_t chunk_size, size_t chunks, void *out) {
		unsigned int segment_size = chunk_size * chunks;
		
		std::stringstream* out_stream = (std::stringstream*)out;
		
		std::string data_string;
		data_string.resize(segment_size);
		data_string.assign((char*)data, segment_size);
		
		*out_stream << data_string;
		
		return segment_size;
	}
	
	bool download(const std::string& url,  
				std::string* output,
				std::vector<std::pair<std::string, std::string>>* get_args=NULL) {
		CURL* c = curl_easy_init();
		
		std::stringstream request;
		request << url;
		if(get_args != NULL && get_args->size() > 0) {
			request << "?";
			for(int i=0; i<get_args->size(); i++) {
				if(i!=0)
					request << "&";
				request << url_encode((*get_args)[i].first) << "=" 
						<< url_encode((*get_args)[i].second);
			}
		}
		
		std::cout << ": Requesting " << request.str() << std::endl;
		curl_easy_setopt(c, CURLOPT_URL, request.str().c_str());
		curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
		
		std::stringstream output_stream;
		
		curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, download_write);
		curl_easy_setopt(c, CURLOPT_WRITEDATA, (void *)&output_stream);
		
		CURLcode result = curl_easy_perform(c);
		
		if(result != CURLE_OK) {
			std::cout << ": Download error: " << curl_easy_strerror(result) << std::endl;
			return false;
		}
		
		curl_easy_cleanup(c);
		
		*output = output_stream.str();
		
		return true;
	}
}
