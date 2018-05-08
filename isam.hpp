// Daniel Crha
#ifndef ISAM_HPP
#define ISAM_HPP

#include <map>
#include <set>
#include "block_provider.hpp"


namespace isam_impl
{
	// Structure of block in memory:
	// 1x 8b - count of items currently in the block
	// 1x 8b - ID of next block
	// capacity x sizeof(TKey, TValue) - payload

	template<class TKey, class TValue>
	struct isam_record
	{
	public:
		TKey key;
		TValue val;

		isam_record() {}
		isam_record(TKey k) : key(k), val() {} // Block stored in memory

		friend bool operator<(const isam_record& lhs, const isam_record& rhs)
		{
			return lhs.key < rhs.key;
		}
	};



	template<class TKey, class TValue>
	struct isam_block
	{
	public:
		size_t count;
		void* block;
		size_t next;
		size_t idx;

		isam_block(size_t block_idx) : idx(block_idx) // Block stored in provider
		{
			if (block_idx == 0) return;
			block = block_provider::load_block(block_idx);
			auto bptr = reinterpret_cast<size_t*>(block);
			count = *bptr; ++bptr;
			next = *bptr;
		}
	};

	// starting at p, moves count records to the right by one
	template<class TKey, class TValue>
	void shift(isam_record<TKey, TValue>* p, size_t count)
	{
		isam_record<TKey, TValue> last, next;
		last = *p; ++p;
		for (size_t i = 0; i < count; ++i)
		{
			next = *p;
			*p = last;
			last = next;
			++p;
		}
	}
}

// TKey: simple value type, no duplicates, comparable: operator<
// TValue: default constructible, assume reasonable usage
template <class TKey, class TValue>
class isam
{
public:
	TValue& operator[](TKey key)
	{
		// FIXME: iterator and indexer writes at the same time?

		// find appropriate block in the primary file using the index
		auto block = _index.lower_bound(key);

		// in case the key exists in the container, returns the value
		// check overflow space first
		auto oflow_result = _oflow.find(isam_impl::isam_record<TKey, TValue>(key));
		if (oflow_result != _oflow.end())
		{
			const TValue& ref = (*oflow_result).val;
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

	isam(size_t block_size, size_t oflow_size) : _block_size(block_size), _oflow_size(oflow_size), _current_block(0), _block_real_size(16 + (block_size * sizeof(isam_impl::isam_record<TKey, TValue>)))
	{
		_oflow = std::set<isam_impl::isam_record<TKey, TValue>>();
	}

	class isam_iter
	{
		// TODO
	};

	isam_iter begin()
	{
		// TODO
	}

	isam_iter end()
	{
		// TODO
	}

private:
	std::map<TKey, size_t> _index = std::map<TKey, size_t>(); // Maps TKeys to block IDs
	std::set<isam_impl::isam_record<TKey, TValue>> _oflow; // TKeys are guaranteed to not contain duplicates
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
			auto block = _index.upper_bound(elem.key);
			if (block == _index.end()) // we are inserting a key that is larger than all existing ones
			{
				if (_index.rbegin() == _index.rend()) // the index is empty -> create new block
				{
					size_t new_block = block_provider::create_block(_block_real_size);
					_current_block = isam_impl::isam_block<TKey, TValue>(new_block);
					auto new_elem = add_to_current_block(elem.key);
					new_elem = elem.val;
					return;
				}
				// try to insert into the last block and increase its maximum
				auto last_block = _index.rbegin();
				load_block((*last_block).second);
				if (_current_block.count < _block_size) // there is room in the block
				{
					auto new_elem = add_to_current_block(elem.key);
					new_elem = elem.val;
					// increase the key of this block in the index
					auto nh = _index.extract(_index.rbegin());
					nh.key() = elem.key;
					_index.insert(move(nh));
				}
				else // no room -> create new block
				{
					size_t new_block = block_provider::create_block(_block_real_size);
					load_block(new_block);
					auto new_elem = add_to_current_block(elem.key);
					new_elem = elem.val;
					return;
				}
			}
			else // check if there is space for insertion
			{

			}

			// if not, create a new block
			// select median of all elements of full block + new elem
			// split the elements in half into the new blocks
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
		_oflow.insert(isam_impl::isam_record<TKey, TValue>(key));
		++_oflow_count;
		const TValue& ref = (*(_oflow.find(isam_impl::isam_record<TKey, TValue>(key)))).val;
		return const_cast<TValue&>(ref); // val is not used for sorting -> const_cast will not break the set
	}

	// tries to retrieve given value from the current block, returns nullptr if it fails
	TValue* try_get_value(TKey key)
	{
		auto stp = reinterpret_cast<size_t*>(_current_block.block);
		stp += 2; // skip header of the block
		auto payload_ptr = reinterpret_cast<isam_impl::isam_record<TKey, TValue>*>(stp);
		isam_impl::isam_record<TKey, TValue> rec;
		for (size_t i = 0; i < _current_block.count; ++i)
		{
			rec = *payload_ptr;
			if (!(key < rec.key) && !(rec.key < key)) // key was found -> skip TKey and return its TValue
			{
				auto key_p = reinterpret_cast<TKey*>(payload_ptr);
				++key_p;
				return reinterpret_cast<TValue*>(key_p);
			}
			++payload_ptr;
		}
		return nullptr;
	}

	TValue& put_record(isam_impl::isam_record<TKey, TValue>* p, TKey key)
	{
		auto key_p = reinterpret_cast<TKey*>(p);
		*key_p = key;
		++key_p;
		auto val_p = reinterpret_cast<TValue*>(key_p);
		new (val_p) TValue(); // in-place construction
		_current_block.count += 1;
		return *val_p;
	}

	// presumes that there is space in the block for inserting (undefined behavior for full block)
	TValue& add_to_current_block(TKey key)
	{
		auto stp = reinterpret_cast<size_t*>(_current_block.block);
		++(*stp); // increase count of items in this block
		stp += 2; // skip block header
		size_t new_elem_pos;
		auto payload_ptr = reinterpret_cast<isam_impl::isam_record<TKey, TValue>*>(stp);
		auto data_start = payload_ptr;
		isam_impl::isam_record<TKey, TValue> rec = *payload_ptr;

		// block is empty -> insert at first positon
		if (_current_block.count == 0)
		{
			return put_record(payload_ptr, key);
		}

		// new item belongs to the front
		if (key < rec.key) new_elem_pos = 0;

		// new item belongs between two other items
		for (size_t i = 0; i < _current_block.count - 1; ++i)
		{
			if ((*payload_ptr).key < key && key < (*(payload_ptr + 1)).key)
			{
				new_elem_pos = i + 1;
				break;
			}
			++payload_ptr;
		}

		// new item belongs at the end
		if ((*payload_ptr).key < key) new_elem_pos = _current_block.count;

		data_start += new_elem_pos;
		isam_impl::shift<TKey, TValue>(data_start, _current_block.count - new_elem_pos); // move other elements to make space for the new one
		return put_record(data_start, key);
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

#endif ISAM_HPP