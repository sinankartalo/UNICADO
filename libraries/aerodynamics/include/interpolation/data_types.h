#ifndef AERODYNAMICS_DATATYPES_H_
#define AERODYNAMICS_DATATYPES_H_

#include <string>
#include <cstdint>
#include <vector>
#include <variant>
#include <CGAL/Epick_d.h>
#include <CGAL/Delaunay_triangulation.h>
#include <CGAL/Triangulation_vertex.h>
#include <Eigen/Dense>

namespace types
{ 
	using MatrixXRd = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

	using VectorXd = Eigen::Matrix<double, Eigen::Dynamic, 1>;

	using key = std::variant<int, std::string, double, char>;
	using value = std::variant<double, std::vector<double>, Eigen::VectorXd>;

	inline long long quantize(double d, double precision = 1e-6) {
		return static_cast<long long>(std::llround(d / precision));
	}

	struct double_equal {
		bool operator()(double a, double b) const {
			return quantize(a) == quantize(b);
		}
	};

	struct double_hash {
		std::size_t operator()(double d) const {
			return std::hash<long long>{}(quantize(d));
		}
	};

	struct key_equal {
		bool operator()(const key& a, const key& b) const {
			if (a.index() != b.index())
				return false;

			if (std::holds_alternative<int>(a))
			{
				return std::get<int>(a) == std::get<int>(b);
			}
			else if (std::holds_alternative<std::string>(a))
			{
				return std::get<std::string>(a) == std::get<std::string>(b);
			}
			else if (std::holds_alternative<double>(a))
			{
				return double_equal{}(std::get<double>(a),std::get<double>(b));
			}
			else if (std::holds_alternative<char>(a))
			{
				return std::get<char>(a) == std::get<char>(b);
			}

			return false;
		}
	};

	struct f_hashmap {
		std::size_t operator() (const key& subkey) const {
			if (std::holds_alternative<int>(subkey))
			{
				return std::hash<int>{}(std::get<int>(subkey));
			}
			else if (std::holds_alternative<std::string>(subkey))
			{
				return std::hash<std::string>{}(std::get<std::string>(subkey));
			}
			else if (std::holds_alternative<double>(subkey))
			{
				return double_hash{}(std::get<double>(subkey));
			}
			return 0;
		}
	};

	using Kernel = CGAL::Epick_d<CGAL::Dynamic_dimension_tag>;
	using Point_CGAL = Kernel::Point;

	using PropertyType = std::unordered_map<key, value, f_hashmap, key_equal>;

	using Vb = CGAL::Triangulation_vertex<Kernel, PropertyType, CGAL::Default>;
	using Cb = CGAL::Triangulation_full_cell<Kernel, CGAL::No_full_cell_data, CGAL::Default>;
	using TDS = CGAL::Triangulation_data_structure<CGAL::Dynamic_dimension_tag, Vb, Cb >;

	using Delaunay = CGAL::Delaunay_triangulation<Kernel, TDS>;

	inline auto get_scalar(const value& val) -> double
	{
		if (auto double_ptr = std::get_if<double>(&val))
		{
			return *double_ptr;
		}
		else if (auto vectorXd_ptr = std::get_if<Eigen::VectorXd>(&val))
		{
			return (*vectorXd_ptr)[0];
		}
		auto std_vector_ptr = std::get_if<std::vector<double>>(&val);

		return (*std_vector_ptr)[0];
	}

	inline auto get_vector(const value& val) -> types::VectorXd
	{
		std::vector<double> vec = {};

		if (auto vectorXd_ptr = std::get_if<types::VectorXd>(&val))
		{
			return *vectorXd_ptr;
		}
		else if (auto double_ptr = std::get_if<double>(&val))
		{
			vec = { *double_ptr };
		}
		else
		{
			auto std_vector_ptr = std::get_if<std::vector<double>>(&val);
			vec = *std_vector_ptr;
		}

		Eigen::Map<const types::VectorXd> column(vec.data(), vec.size());

		return column;
	}
}

#endif