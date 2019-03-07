#ifndef UTILS_TARRAYPOD_H
#define UTILS_TARRAYPOD_H

#include <string>
#include <assert.h>
// selfdefine array container
template<typename TYPE, size_t SIZE>
class TArrayPod
{
private:
	typedef TArrayPod<TYPE, SIZE> selfType;
	size_t m_nCapicity;    
	TYPE m_Array[SIZE];
	TYPE* m_pData;
	size_t m_nSize;

public:
	TArrayPod() : m_nCapicity(SIZE), m_pData(m_Array), m_nSize(0) {}

	TArrayPod(const selfType& src)
	{
		if (this == &src)
		{
			return;
		}

		m_nSize = src.m_nSize;
		if (m_nSize <= SIZE)
		{
			m_pData = m_Array;
			m_nCapicity = SIZE;
		}
		else
		{
			m_nCapicity = src.m_nCapicity;
			m_pData = new TYPE[m_nCapicity];
		}

		memcpy(m_pData, src.m_pData, m_nSize * sizeof(TYPE));
	}

	virtual ~TArrayPod()
	{
		if (m_nCapicity > m_nSize)
		{
			delete[] m_pData;
		}
	}

	selfType& operator=(const selfType& src)
	{
		if (this == &src)
		{
			return *this;
		}

		selfType temp(src);
		swap(temp);
		return *this;
	}

	TYPE& operator[](const size_t index)
	{
		assert(index < m_nSize);
		return m_pData[index];
	}

	const TYPE& operator[](const size_t index) const
	{
		assert(index < m_nSize);
		return m_pData[index];
	}

	void swap(selfType& src)
	{
		size_t temp_size = src.m_nSize;
		size_t temp_capicity = src.m_nCapicity;
		TYPE* temp_data = src.m_pData;
		TYPE temp_array[SIZE];

		if (temp_capicity <= SIZE)
		{
			memcpy(temp_array, src.m_Array, temp_size * sizeof(TYPE));
		}

		src.m_nSize = m_nSize;
		src.m_nCapicity = m_nCapicity;
		if (m_nCapicity <= SIZE)
		{
			memcpy(src.m_Array, m_Array, m_nSize * sizeof(TYPE));
			src.m_pData = src.m_Array;
		}
		else
		{
			src.m_pData = m_pData;
		}

		m_nSize = temp_size;
		m_nCapicity = temp_capicity;
		if (m_nCapicity <= SIZE)
		{
			memcpy(m_Array, temp_array, m_nSize * sizeof(TYPE));
			m_pData = m_Array;
		}
		else
		{
			m_pData = temp_data;
		}
	}

	bool empty() const
	{
		return (0 == m_nSize);
	}

	size_t size() const
	{
		return m_nSize;
	}

	const TYPE* Data() const
	{
		return m_pData;
	}

	TYPE* Data()
	{
		return m_pData;
	}
	
	void push_back(const TYPE& value)
	{
		if (m_nSize == m_nCapicity)
		{
			size_t new_size = m_nCapicity << 1;
			TYPE* pNew = new TYPE[new_size];
			if (!pNew)
			{
				return;
			}

			memcpy(pNew, m_pData, m_nSize * sizeof(TYPE));
			if (m_nCapicity > SIZE)
			{
				delete[] m_pData;

			}
			m_pData = pNew;
			m_nCapicity = new_size;
		}

		m_pData[m_nSize++] = m_pData;
	}

	TYPE& pop_back()
	{
		assert(m_nSize > 0);
		return m_pData[--m_nSize];
	}

	TYPE& back()
	{
		assert(m_nSize > 0);
		return m_pData[m_nSize - 1];
	}

	const TYPE& back() const
	{
		assert(m_nSize > 0);
		return m_pData[m_nSize - 1];
	}

	void reserve(size_t size)
	{
		if (m_nCapicity < size)
		{
			TYPE* pNew = new TYPE[size];
			assert(pNew);
			memcpy(pNew, m_pData, m_nSize * sizeof(TYPE));
			if (m_nCapicity > SIZE)
			{
				delete[] m_pData;
			}
			m_pData = pNew;
			m_nCapicity = size;
		}
	}

	void resize(size_t size)
	{
		if (size > m_nCapicity)
		{
			size_t new_size = m_nCapicity << 1;
			if (new_size < size)
			{
				new_size = size;
			}

			TYPE* pNew = new TYPE[new_size];
			assert(pNew);
			memcpy(pNew, m_pData, m_nSize * sizeof(TYPE));
			
			if (m_nCapicity > SIZE)
			{
				delete[] m_pData;
			}

			m_pData = pNew;
			m_nCapicity = new_size;
		}

		m_nSize = size;
	}

	void resize(size_t size, const TYPE& value)
	{
		size_t tmp = m_nSize;
		resize(size);
		for (size_t i = tmp; i < size; ++i)
		{
			m_pData[i] = value;
		}
	}

	void insert(size_t index, const TYPE& value)
	{
		assert(index <= m_nSize);
		resize(m_nSize + 1);
		TYPE* p = m_pData + index;
		memmove_s(p + 1, (m_nSize - index - 1) * sizeof(TYPE), p, (m_nSize - index - 1) * sizeof(TYPE));
		*p = value;
	}

	void remove(size_t index)
	{
		assert(index < m_nSize);
		TYPE* p = m_pData + 1;
		memmove_s(p, (m_nSize - index - 1) * sizeof(TYPE), p + 1, (m_nSize - index -1) * sizeof(TYPE));
		m_nSize--;
	}

	void remove_from(size_t start, size_t amount)
	{
		assert(start < m_nSize && start + amount < m_nSize);
		TYPE* p = m_pData + start;
		TYPE* pEnd = m_pData + start + amount;
		memmove_s(p, (m_nSize - start - amount) * sizeof(TYPE), pEnd, (m_nSize - start - amount) * sizeof(TYPE));
		m_nSize -= amount;
	}

	void clear()
	{
		m_nSize = 0;
	}

	size_t get_memory_usage() const
	{
		size_t size = sizeof(selfType);
		if (m_nCapicity > SIZE)
		{
			size += sizeof(TYPE) * m_nCapicity;
		}

		return size;
	}
};
#endif // !UTILS_TARRAYPOD_H


