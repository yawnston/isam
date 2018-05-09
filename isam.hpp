// Daniel Crha
#ifndef ISAM_HPP
#define ISAM_HPP

#include <map>
#include <set>
#include "block_provider.hpp"


namespace isam_impl
{
	template<class TKey, class TValue>
	struct ComparePairFst
	{
		bool operator ()(const std::pair<TKey, TValue>& p1, const std::pair<TKey, TValue>& p2) const
		{
			return p1.first < p2.first;
		}
	};

	// Structure of block in memory:
	// 1x 8b - count of items currently in the block
	// 1x 8b - ID of next block
	// capacity x sizeof(TKey, TValue) - payload

	template<class TKey, class TValue>
	struct isam_block
	{
	public:
		size_t count;
		void* block;
		size_t next;
		size_t idx;

		void set_next(size_t n)
		{
			auto bptr = reinterpret_cast<size_t*>(block);
			++bptr;
			*bptr = n;
			next = n;
		}

		isam_block(size_t block_idx) : idx(block_idx) // Block stored in provider
		{
			if (block_idx == 0) return;
			block = block_provider::load_block(block_idx);
			auto bptr = reinterpret_cast<size_t*>(block);
			count = *bptr; ++bptr;
			next = *bptr;
		}
	};

	template<class TKey, class TValue>
	TValue& put_record(std::pair<TKey, TValue>* p, TKey key)
	{
		auto key_p = reinterpret_cast<TKey*>(p);
		*key_p = key;
		++key_p;
		auto val_p = reinterpret_cast<TValue*>(key_p);
		new (val_p) TValue(); // in-place construction
		return *val_p;
	}

	// starting at p, moves count records to the right by one
	template<class TKey, class TValue>
	void shift(std::pair<TKey, TValue>* p, size_t count)
	{
		std::pair<TKey, TValue> last, next;
		last = *p; ++p;
		for (size_t i = 0; i < count; ++i)
		{
			next = *p;
			//*p = last;
			put_record<TKey, TValue>(p, last.first) = last.second;
			last = next;
			++p;
		}
	}

	template<class TKey, class TValue>
	TValue& insert(void* block, TKey key)
	{
		auto stp = reinterpret_cast<size_t*>(block);
		size_t count = *stp;
		++(*stp); // increase count of items in this block
		stp += 2; // skip block header
		size_t new_elem_pos = 0;
		auto payload_ptr = reinterpret_cast<std::pair<TKey, TValue>*>(stp);
		auto data_start = payload_ptr;
		std::pair<TKey, TValue> rec = *payload_ptr;

		// block is empty -> insert at first positon
		if (count == 0)
		{
			return put_record(payload_ptr, key);
		}

		// new item belongs to the front
		if (key < rec.first) new_elem_pos = 0;

		// new item belongs between two other items
		for (size_t i = 0; i < count - 1; ++i)
		{
			if ((*payload_ptr).first < key && key < (*(payload_ptr + 1)).first)
			{
				new_elem_pos = i + 1;
				break;
			}
			++payload_ptr;
		}

		// new item belongs at the end
		if (new_elem_pos == 0 && (*payload_ptr).first < key) new_elem_pos = count;

		data_start += new_elem_pos;
		shift<TKey, TValue>(data_start, count - new_elem_pos); // move other elements to make space for the new one
		return put_record(data_start, key);
	}

	// splits the elements from the first block between itself and the second one (empty), including a new element with key, returning a reference to the new elem's value
	template<class TKey, class TValue>
	TValue& split(void* fst, void* snd, TKey key)
	{
		auto fst_p = reinterpret_cast<size_t*>(fst); auto snd_p = reinterpret_cast<size_t*>(snd);
		size_t count_fst = *fst_p; fst_p += 2; snd_p += 2;
		auto fst_payload = reinterpret_cast<std::pair<TKey, TValue>*>(fst_p);
		auto snd_payload = reinterpret_cast<std::pair<TKey, TValue>*>(snd_p);
		fst_payload += ((count_fst / 2) + (count_fst % 2) - 1);
		size_t copy_start_index; bool new_elem_left;
		if (key < (*fst_payload).first) // copy the "middle" element as well because the new one is going on the left
		{
			copy_start_index = (count_fst / 2) + (count_fst % 2) - 1;
			new_elem_left = true;
		}
		else // leave the "middle" element because the new one is going on the right
		{
			copy_start_index = (count_fst / 2) + (count_fst % 2);
			new_elem_left = false;
			++fst_payload;
		}

		// copy the appropriate amount of elements from left to right
		size_t right_count = 0;
		for (size_t i = copy_start_index; i < count_fst; ++i)
		{
			//*snd_payload = *fst_payload;
			put_record<TKey, TValue>(snd_payload, fst_payload->first) = fst_payload->second;
			++right_count;
			++snd_payload; ++fst_payload;
		}
		*(reinterpret_cast<size_t*>(fst)) -= right_count;
		*(reinterpret_cast<size_t*>(snd)) = right_count;

		// insert the new element into the right block
		void* new_elem_block;
		if (new_elem_left) new_elem_block = fst;
		else new_elem_block = snd;

		return insert<TKey, TValue>(new_elem_block, key);
	}

	// get maximum key from the given non-empty block
	template<class TKey, class TValue>
	TKey get_max(void* block)
	{
		auto stp = reinterpret_cast<size_t*>(block);
		size_t count = *stp;
		stp += 2;
		auto payload_ptr = reinterpret_cast<std::pair<TKey, TValue>*>(stp);
		payload_ptr += (count - 1);
		return (*payload_ptr).first;
	}

	template<class TKey, class TValue>
	std::pair<TKey, TValue>* to_pair_ptr(typename std::set<std::pair<TKey, TValue>, isam_impl::ComparePairFst<TKey, TValue>>::iterator it)
	{
		return const_cast<std::pair<TKey, TValue>*>(&(*it));
	}
}

// TKey: simple value type, no duplicates, comparable: operator<
// TValue: default constructible, assume reasonable usage
template <class TKey, class TValue>
class isam
{
public:
	TValue & operator[](TKey key)
	{
		// find appropriate block in the primary file using the index
		auto block = _index.lower_bound(key);

		// in case the key exists in the container, returns the value
		// check overflow space first
		auto oflow_result = _oflow.find(std::pair<TKey, TValue>(key, TValue())); // !!!
		if (oflow_result != _oflow.end())
		{
			const TValue& ref = (*oflow_result).second;
			return const_cast<TValue&>(ref); // val is not used for sorting -> const_cast will not break the set
		}

		if (block != _index.end()) // FIXME: inserting key that is larger than all existing ones
		{
			load_block((*block).second);
			auto result = try_get_value(key);
			if (result != nullptr) return *result;

			// in case the key does not exist in the container
			// if the block is not full insert new record to the block with default value
			if (_current_block.count < _block_size)
			{
				return add_to_current_block(key);
			}
		}
		// else insert new record to the overflow space (happens when there is no room or the isam is empty)
		return add_to_oflow(key);
	}

	isam(size_t block_size, size_t oflow_size) : _block_size(block_size), _oflow_size(oflow_size), _current_block(0)
	{
		_oflow = std::set<std::pair<TKey, TValue>, isam_impl::ComparePairFst<TKey, TValue>>();
		_block_real_size = 16 + (block_size * sizeof(std::pair<TKey, TValue>));
	}

	isam(const isam&) = delete;

	~isam()
	{
		if (_current_block.idx != 0) push_current_block();
	}

	class isam_iter
	{
	public:
		typedef isam_iter self_type;
		typedef std::pair<TKey, TValue> value_type;
		typedef std::pair<TKey, TValue>& reference;
		typedef std::pair<TKey, TValue>* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef ptrdiff_t difference_type;

		isam_iter & operator++()
		{
			// find out whether to move inside overflow or inside the block
			bool oflow_move; bool check_both = true;
			if (_index_in_oflow == _oflow_size) { oflow_move = false; check_both = false; }
			if (_block.idx == 0) { oflow_move = true; check_both = false; }
			if (check_both)
			{
				oflow_move = (_oflow_it->first < _block_ptr->first);
			}

			if (oflow_move)
			{
				++_index_in_oflow;
				++_oflow_it;
			}
			else
			{
				if (_index_in_block == _block.count - 1) load_next_block();
				else
				{
					++_index_in_block;
					++_block_ptr;
				}
			}
			// if we can't move, transform this iterator into a past-the-end iterator
			if (_block.idx == 0 && _index_in_oflow == _oflow_size)
			{
				_index_in_block = 1;
				_index_in_oflow = 0;
			}
			return *this;
		}

		isam_iter operator++(int) // postfix variant
		{
			isam_iter result(*this);
			++(*this);
			return result;
		}

		bool operator ==(const isam_iter& b) const
		{
			return (b._block.idx == _block.idx && b._index_in_block == _index_in_block && b._index_in_oflow == _index_in_oflow);
		}

		bool operator !=(const isam_iter& b) const
		{
			return !operator==(b);
		}

		isam_iter& operator=(const isam_iter& i)
		{
			_index_in_block = i._index_in_block;
			_index_in_oflow = i._index_in_oflow;
			_oflow_size = i._oflow_size;
			_oflow_it = i._oflow_it;
			if (_block.idx != 0) block_provider::store_block(_block.idx, _block.block);
			_block = isam_impl::isam_block<TKey, TValue>(i._block.idx);
			if (_block.idx != 0) // set the block ptr to the correct element
			{
				size_t* skipper = (reinterpret_cast<size_t*>(_block.block) + 2);
				_block_ptr = reinterpret_cast<std::pair<TKey, TValue>*>(skipper);
				_block_ptr += _index_in_block;
			}
			return *this;
		}

		std::pair<TKey, TValue>& operator *() const
		{
			return *(operator->());
		}

		std::pair<TKey, TValue>* operator ->() const
		{
			if (_block.idx == 0) return isam_impl::to_pair_ptr<TKey, TValue>(_oflow_it);
			if (_index_in_oflow == _oflow_size) return _block_ptr;
			if (_oflow_it->first < _block_ptr->first) return isam_impl::to_pair_ptr<TKey, TValue>(_oflow_it);
			return _block_ptr;
		}

		isam_iter() : _block(0), _index_in_block(1), _index_in_oflow(0) {} // end iterator constructor

		isam_iter(const isam_iter& i) : isam_iter() // copy constructor
		{
			operator=(i);
		}

		isam_iter(size_t block, typename std::set<std::pair<TKey, TValue>, isam_impl::ComparePairFst<TKey, TValue>>::iterator o_it, size_t oflow_size) : _block(block), _index_in_block(0), _index_in_oflow(0), _oflow_size(oflow_size)
		{
			if (block == 0 && oflow_size == 0) _index_in_block = 1;
			_oflow_it = o_it;
			size_t* skipper = (reinterpret_cast<size_t*>(_block.block) + 2);
			_block_ptr = reinterpret_cast<std::pair<TKey, TValue>*>(skipper);
		}

		~isam_iter()
		{
			if (_block.idx != 0) block_provider::store_block(_block.idx, _block.block);
		}

	private:
		isam_impl::isam_block<TKey, TValue> _block;
		size_t _index_in_block;
		size_t _index_in_oflow;
		std::pair<TKey, TValue>* _block_ptr;
		typename std::set<std::pair<TKey, TValue>, isam_impl::ComparePairFst<TKey, TValue>>::iterator _oflow_it;
		size_t _oflow_size;

		void load_next_block()
		{
			size_t next_id = _block.next;
			if (_block.idx != 0) block_provider::store_block(_block.idx, _block.block);
			_block = isam_impl::isam_block<TKey, TValue>(next_id);
			if (next_id == 0) return;
			_index_in_block = 0;
			size_t* skipper = (reinterpret_cast<size_t*>(_block.block) + 2);
			_block_ptr = reinterpret_cast<std::pair<TKey, TValue>*>(skipper);
		}
	};

	isam_iter begin()
	{
		load_block(0); // push any current changes in the loaded block before creating the iterator
		size_t id;
		auto first = _index.begin();
		id = first == _index.end() ? 0 : first->second;
		return isam_iter(id, _oflow.begin(), _oflow_count);
	}

	isam_iter end()
	{
		return isam_iter(); // block idx == 0 && idx_in_block == 1 && idx_in_oflow == 0 indicates the end() iterator
	}

private:
	std::map<TKey, size_t> _index = std::map<TKey, size_t>(); // Maps TKeys to block IDs
	std::set<std::pair<TKey, TValue>, isam_impl::ComparePairFst<TKey, TValue>> _oflow; // TKeys are guaranteed to not contain duplicates
	size_t _block_size; // Number of (TKey, TValue) records
	size_t _block_real_size; // Number of bytes
	size_t _oflow_size;
	size_t _oflow_count = 0;
	isam_impl::isam_block<TKey, TValue> _current_block;

	// insert the overflow records into the main file
	void push_oflow()
	{
		for (auto&& elem : _oflow)
		{
			// check if there is space in an existing block
			auto block = _index.upper_bound(elem.first);
			if (block == _index.end()) // we are inserting a key that is larger than all existing ones
			{
				if (_index.rbegin() == _index.rend()) // the index is empty -> create new block
				{
					size_t new_block = block_provider::create_block(_block_real_size);
					_current_block = isam_impl::isam_block<TKey, TValue>(new_block);
					_current_block.set_next(0);
					TValue& new_elem = add_to_current_block(elem.first);
					new_elem = elem.second;
					_index[elem.first] = new_block;
					continue;
				}
				// try to insert into the last block and increase its maximum
				auto last_block = _index.rbegin();
				load_block((*last_block).second);
				if (_current_block.count < _block_size) // there is room in the block
				{
					TValue& new_elem = add_to_current_block(elem.first);
					new_elem = elem.second;
					// increase the key of this block in the index
					_index.erase((++(_index.rbegin())).base());
					_index[elem.first] = _current_block.idx;
					continue;
				}
				else // no room -> create new block
				{
					size_t new_block = block_provider::create_block(_block_real_size);
					_current_block.set_next(new_block);
					load_block(new_block);
					_current_block.set_next(0);
					TValue& new_elem = add_to_current_block(elem.first);
					new_elem = elem.second;
					_index[elem.first] = new_block;
					continue;
				}
			}
			else // check if there is space for insertion
			{
				load_block((*block).second);
				if (_current_block.count < _block_size) // there is room in the block
				{
					TValue& new_elem = add_to_current_block(elem.first);
					new_elem = elem.second;
					continue;
				}
				else // no room -> create new block and split elements in half
				{
					TKey curr_key = isam_impl::get_max<TKey, TValue>(_current_block.block);
					size_t new_block_id = block_provider::create_block(_block_real_size);
					isam_impl::isam_block<TKey, TValue> new_block(new_block_id);
					new_block.set_next(_current_block.next);
					_current_block.set_next(new_block_id);
					TValue& new_item = isam_impl::split<TKey, TValue>(_current_block.block, new_block.block, elem.first);
					new_item = elem.second;
					// increase the key of this block in the index
					// insert new_block into the index with the proper key
					TKey max_l = isam_impl::get_max<TKey, TValue>(_current_block.block);
					TKey max_r = isam_impl::get_max<TKey, TValue>(new_block.block);
					_index.erase(curr_key);
					_index[max_l] = _current_block.idx;
					_index[max_r] = new_block_id;
					block_provider::store_block(new_block_id, new_block.block);
					continue;
				}
			}
		}
		_oflow_count = 0; _oflow.clear();
	}

	TValue& add_to_oflow(TKey key)
	{
		// TODO: what to do if oflow_size == 0?
		if (_oflow_count == _oflow_size)
		{
			push_oflow();
		}
		_oflow.insert(std::pair<TKey, TValue>(key, TValue()));
		++_oflow_count;
		const TValue& ref = (*(_oflow.find(std::pair<TKey, TValue>(key, TValue())))).second;
		return const_cast<TValue&>(ref); // val is not used for sorting -> const_cast will not break the set
	}

	// tries to retrieve given value from the current block, returns nullptr if it fails
	TValue* try_get_value(TKey key) const
	{
		auto stp = reinterpret_cast<size_t*>(_current_block.block);
		stp += 2; // skip header of the block
		auto payload_ptr = reinterpret_cast<std::pair<TKey, TValue>*>(stp);
		std::pair<TKey, TValue> rec;
		for (size_t i = 0; i < _current_block.count; ++i)
		{
			rec = *payload_ptr;
			if (!(key < rec.first) && !(rec.first < key)) // key was found -> skip TKey and return its TValue
			{
				auto key_p = reinterpret_cast<TKey*>(payload_ptr);
				++key_p;
				return reinterpret_cast<TValue*>(key_p);
			}
			++payload_ptr;
		}
		return nullptr;
	}

	// presumes that there is space in the block for inserting (undefined behavior for full block)
	TValue& add_to_current_block(TKey key)
	{
		_current_block.count += 1;
		return isam_impl::insert<TKey, TValue>(_current_block.block, key);
	}

	void load_block(size_t id)
	{
		if (id != _current_block.idx)
		{
			if (_current_block.idx != 0) push_current_block();
			_current_block = isam_impl::isam_block<TKey, TValue>(id);
		}
	}

	// writes the current block back into the provider
	void push_current_block()
	{
		block_provider::store_block(_current_block.idx, _current_block.block);
	}
};

#endif // ISAM_HPP