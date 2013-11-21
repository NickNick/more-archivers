#include <iostream>
#include <sstream>
#include <cstring>

#include <cstddef>

#include <boost/archive/detail/common_oarchive.hpp>
#include <boost/archive/detail/common_iarchive.hpp>
#include <boost/archive/detail/register_archive.hpp>
#include <boost/serialization/item_version_type.hpp>

#include <msgpack.hpp>

namespace more_archivers {
	struct msgpack_oarchive : boost::archive::detail::common_oarchive<msgpack_oarchive> {
		msgpack_oarchive(msgpack::sbuffer& buffer_)
			: buffer(buffer_)
		{}

	private:
		friend class boost::archive::save_access;
		friend class boost::archive::detail::interface_oarchive<msgpack_oarchive>;

		msgpack::sbuffer& buffer;
		msgpack::packer<msgpack::sbuffer> packer{&buffer};

		template <typename T>
		void general_save(T const& t){
			packer.pack(t);
		}

		template <typename T>
		void save_override(T const& t, int){
			general_save(t);
		}

		template <typename T>
		void save_override(boost::serialization::nvp<T> const& x, int){
			packer.pack_map(1);
			packer.pack(std::string(x.name()));
			packer.pack(x.value());
		}

		void general_save(boost::archive::version_type const& ){}
		void general_save(boost::archive::object_id_type const& ){}
		void general_save(boost::archive::object_reference_type const& ){}
		void general_save(boost::archive::class_id_type const& ){}
		void general_save(boost::archive::class_id_optional_type const&){}
		void general_save(boost::archive::class_id_reference_type const& ){}
		void general_save(boost::archive::class_name_type const& ){}
		void general_save(boost::archive::tracking_type const& ){}

		void general_save(boost::serialization::collection_size_type const& size){
			packer.pack_array(size);
		}

		void general_save(boost::serialization::item_version_type const&){}

	public:
		void save_binary(void *address, std::size_t count){
			packer.pack_raw_body((char*)address, count);
		}
	};

	struct msgpack_iarchive : boost::archive::detail::common_iarchive<msgpack_iarchive> {
		msgpack_iarchive(msgpack::sbuffer const& buffer)
		{
			unpacker.reserve_buffer(buffer.size());
			std::copy(buffer.data(), buffer.data() + buffer.size(), unpacker.buffer());
			unpacker.buffer_consumed(buffer.size());
		}

	private:
		friend class boost::archive::load_access;
		friend class boost::archive::detail::interface_iarchive<msgpack_iarchive>;

		msgpack::unpacker unpacker;
		msgpack::unpacked result;

		template <typename T>
		void unpack_msgpack(T& t, msgpack::object& object){
			object.convert(&t);
		}

		template <typename T>
		void general_load(T& t){
			unpacker.next(&result);
			unpack_msgpack(t, result.get());
		}

		template <typename T>
		void general_load(boost::serialization::nvp<T> const& x){
			unpacker.next(&result);
			auto const& obj = result.get();
			if(obj.type != msgpack::type::MAP){
				std::stringstream str; str << obj.type;
				throw std::runtime_error("Expected a map (for name-value-pair), but got a " + str.str());
			}
			auto const& map = result.get().via.map;
			if(map.size != 1){
				throw std::runtime_error("Expected a single-size-map for name-value-pair, but got a size " + std::to_string(map.size));
			}

			std::string key;
			unpack_msgpack(key, map.ptr->key);

			if(key != x.name()){
				throw std::runtime_error(std::string("Expected name '") + x.name() +"' but got '" + key +  "'");
			}

			unpack_msgpack(x.value(), map.ptr->val);
		}

		template <typename T>
		void load_override(T& x, int){
			general_load(x);
		}

		void general_load(boost::archive::class_name_type& n){
			std::cout << "Reading an " << n << std::endl;
		}

		void general_load(boost::archive::version_type& ){}
		void general_load(boost::archive::object_id_type& ){}
		void general_load(boost::archive::object_reference_type& ){}
		void general_load(boost::archive::class_id_type& ){}
		void general_load(boost::archive::class_id_optional_type&){}
		void general_load(boost::archive::class_id_reference_type& ){}
		void general_load(boost::archive::tracking_type& ){}

		void general_load(boost::serialization::collection_size_type& x){
			unpacker.next(&result);
			x = result.get().as<std::size_t>();
		}

		void general_load(boost::serialization::item_version_type& ){}

	public:
		void load_binary(void *address, std::size_t count){
			unpacker.next(&result);
			auto const& raw = result.get().via.raw;
			std::copy(raw.ptr, raw.ptr + raw.size, (char*)address);
		}
	};
}

BOOST_SERIALIZATION_REGISTER_ARCHIVE(more_archivers::msgpack_oarchive);
BOOST_SERIALIZATION_REGISTER_ARCHIVE(more_archivers::msgpack_iarchive);
