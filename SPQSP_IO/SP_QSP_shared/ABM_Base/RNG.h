#ifndef __RNG__
#define __RNG__

#include <boost/random.hpp>
#include <boost/serialization/nvp.hpp>

#include <vector>

class RNG
{
public:
	typedef boost::mt19937 RNGType;
	//typedef boost::random::uniform_real_distribution<> UNI_REAL_DIST;
	//typedef boost::variate_generator<RNGType &, UNI_REAL_DIST> UNI_REAL_GEN;
	//typedef boost::random::uniform_int_distribution<> UNI_INT_DIST;
	//typedef boost::variate_generator<RNGType &, UNI_INT_DIST> UNI_INT_GEN;
	typedef boost::random::uniform_01<> UNI_01_DIST;
	typedef boost::variate_generator<RNGType &, UNI_01_DIST> UNI_01_GEN;
	typedef boost::random::normal_distribution<> BOOST_NORMAL_DIST;
	typedef boost::variate_generator<RNGType &, BOOST_NORMAL_DIST > BOOST_NORMAL_GEN;

	typedef boost::random::exponential_distribution<> BOOST_EXP_DIST;
	typedef boost::variate_generator<RNGType &, BOOST_EXP_DIST> BOOST_EXP_GEN;

	//! sample k elements from a vector
	template <typename T>
	void shuffle_first_k(std::vector<T>&, unsigned int k);
	//! sample one element from a vector
	template <typename T>
	int single_draw(const std::vector<T>&);

public: // other algorithms
	//! draw from a vector of probabilities
	int sample_cdf(const std::vector<double>& cdf);
public:
	RNG();
	~RNG();
	//! set seed
	void seed(unsigned int s);
	
	//! get rng
	RNGType& getRNG(void){ return _rng; };
	//! get a random value from uniform distribution (0, 1)
	double get_unif_01();
	//! get a random value from standard normal distribution
	double get_norm_std();
	//! get random normal
	double get_normal(double mean, double sd);
	//! get a random value from exponential distribution
	double get_exponential(double mean);

private:
	friend class boost::serialization::access;
	//! boost serialization
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	RNGType _rng;
	UNI_01_GEN _unif_01_gen;
	BOOST_NORMAL_GEN _norm_std_gen;
	BOOST_EXP_GEN _exponential_1_gen;
};

template<class Archive>
inline void RNG::serialize(Archive & ar, const unsigned int  version){
	ar & BOOST_SERIALIZATION_NVP(_rng);
}

/*! Sample k elements without replacement.
	Fisher-Yates.
	After shuffling, the first k elements of vector will be the sample.
	The order of the k element will be a random permutation.
    Definition of member method template need to be in header file instead of cpp
*/
template <typename T>
inline void RNG::shuffle_first_k(std::vector<T>& v, unsigned int k){
	unsigned int n = v.size();
	if (k <= n)
	{
		for (unsigned int i = 0; i < k; i++){
			std::swap(v[i], v[i + (int)(get_unif_01()*(n - i))]);
		}
	}
	else{
		throw std::invalid_argument("RNG::shuffle_first_k: shuffle out of range");
	}
	return;
}

//! sample one element from a vector
template <typename T>
int RNG::single_draw(const std::vector<T>& v){
	int r = int(get_unif_01() * v.size());
	return r;
}

namespace boost {
	namespace serialization {
		typedef RNG::RNGType RNGType;
		//! boost serialization function for global random number generator (Asymmetric, split).
		template<class Archive>
		inline void serialize(Archive& ar, RNGType & r, const unsigned int version){
			split_free(ar, r, version);
		}
		//! boost serialization for rng: save to xml
		template<class Archive>
		inline void save(Archive& ar, const RNGType & r, const unsigned int){
			std::ostringstream ss;
			ss << r;
			std::string s(ss.str());
			//ar << s;
			ar << boost::serialization::make_nvp("_rng", s);
		}
		//! boost serialization for rng: load from xml
		template<class Archive>
		inline void load(Archive& ar,  RNGType & r, const unsigned int){
			std::string s;
			//ar >> s;
			ar >> boost::serialization::make_nvp("_rng", s);
			std::istringstream ss(s);
			ss >> r;
		}
	}
}


#endif
