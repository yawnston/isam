// Daniel Crha
#ifndef ISAM_HPP
#define ISAM_HPP

#include <map>
#include <vector>
#include "block_provider.hpp"


namespace isam_impl
{
	// Structure of block in memory:
	// 1x 8b - count of items currently in the block
	// 1x 8b - ID of next block
	// capacity x sizeof(TKey, TValue) - payload

	template<class TKey, class TValue>
	struct isam_mem_record
	{
	public:
		TKey key;
		TValue val;

		isam_mem_record(TKey k, TValue v) : key(k), val(v) {} // Block stored in memory
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
			if (block_idx == 0) return; // FIXME: throw exception here?
			block = block_provider::load_block(block_idx);
			auto bptr = reinterpret_cast<size_t*> block;
			count = *bptr; ++bptr;
			next = *bptr;
		}
	};
}

// TKey: simple value type, no duplicates, comparable: operator<
// TValue: default constructible, assume reasonable usage
template <class TKey, class TValue>
class isam
{
public:
	TValue& operator[](TKey key)
	{
		// find appropriate block in the primary file using the index
		auto block = _index.lower_bound(key);

		// in case the key exists in the container, returns the value
		if (block != _index.end())
		{
			if (*block != _current_block.idx) // get a new block if we don't want the one we have right now
			{
				push_current_block();
				_current_block = isam_impl::isam_block<>(*block);
			}
			auto result = try_get_value(key);
			if (result != nullptr) return result;

			// in case the key does not exist in the container
			// if the block is not full insert new record to the block with default value
			if (_current_block.count < _block_size)
			{
				return add_to_current_block(key);
			}
		}
		// else insert new record to the overflow space (happens when there is no room or the isam is empty)
		return add_to_oflow(isam_impl::isam_mem_record<>(key, TValue()));
	}

	isam(size_t block_size, size_t oflow_size) : _block_size(block_size), _oflow_size(oflow_size)
	{
		_oflow = std::vector<>(oflow_size);
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
	std::map<TKey, size_t, std::less<TKey>> _index = std::map<>(); // Maps TKeys to block IDs
	std::vector<isam_impl::isam_mem_record<TKey, TValue>> _oflow;
	size_t _block_size; // Number of (TKey, TValue) records
	size_t _oflow_size;
	size_t _oflow_count = 0;
	isam_impl::isam_block<TKey, TValue> _current_block;

	void push_oflow()
	{
		// TODO
	}

	TValue& add_to_oflow(isam_impl::isam_mem_record<TKey, TValue>&& rec)
	{
		if (_oflow_count == _oflow_size)
		{
			push_oflow();
		}
	}

	// tries to retrieve given value from the current block, returns nullptr if it fails
	TValue& try_get_value(TKey key)
	{

	}

	// presumes that there is space in the block for inserting (undefined behavior for full block)
	TValue& add_to_current_block(TKey key)
	{

	}

	void push_current_block()
	{

	}
};

#endif ISAM_HPP