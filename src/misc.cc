module;

#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <iostream>
#include <curl/curl.h>

export module razor.misc;

export namespace razor {
	typedef unsigned long long int nanotime;
	inline constexpr nanotime NANOS_PER_MILLI = 1'000'000ULL;
	inline constexpr nanotime NANOS_PER_SECOND = 1'000'000'000ULL;
	
	nanotime nanoNow() {
		return std::chrono::nanoseconds(
				std::chrono::high_resolution_clock::now().time_since_epoch()
				).count();
	}
	
	std::string urlEncode(std::string& s) {
		CURL* c = curl_easy_init();
		char* out_raw = curl_easy_escape(c, s.c_str(), s.size());
		std::string out(out_raw);
		curl_free(out_raw);
		curl_easy_cleanup(c);
		return out;
	}
	
	std::string urlDecode(std::string& s) {
		CURL* c = curl_easy_init();
		char* out_raw = curl_easy_unescape(c, s.c_str(), s.size(), NULL);
		std::string out(out_raw);
		curl_free(out_raw);
		curl_easy_cleanup(c);
		return out;
	}
	
	static size_t downloadWrite(void *data, size_t chunk_size, size_t chunks, void *out) {
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
				request << urlEncode((*get_args)[i].first) << "=" 
						<< urlEncode((*get_args)[i].second);
			}
		}
		
		curl_easy_setopt(c, CURLOPT_URL, request.str().c_str());
		curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
		
		std::stringstream output_stream;
		
		curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, downloadWrite);
		curl_easy_setopt(c, CURLOPT_WRITEDATA, (void *)&output_stream);
		
		CURLcode result = curl_easy_perform(c);
		
		if(result != CURLE_OK) {
			return false;
		}
		
		curl_easy_cleanup(c);
		
		*output = output_stream.str();
		
		return true;
	}
}
