#pragma once

#include <mutex>

template<typename T>
class AddableBuffer;

template<typename T>
class SwappableBuffer {
	T* writeBuf;
	size_t writeIndex = 0;
	size_t writeCapacity;
	size_t readCapacity;
	std::mutex readLock;
public:
	T* readData;
	size_t readSize;

	SwappableBuffer(int initialCapacity) : writeCapacity(initialCapacity), writeBuf((T*)malloc(initialCapacity * sizeof(T))),
		readCapacity(initialCapacity), readData((T*)malloc(initialCapacity * sizeof(T))) {
		if (readData == nullptr || writeBuf == nullptr)
			Log::fatal("Could not create SwappableBuffer of size: %d", initialCapacity);
	}

	~SwappableBuffer() {
		lockRead();
		free(writeBuf);
		free(readData);
		unlockRead();
	}

	SwappableBuffer(const SwappableBuffer&) = delete;
	SwappableBuffer& operator=(const SwappableBuffer&) = delete;


	void add(const T& obj) {
		if (writeIndex == writeCapacity) {
			writeCapacity *= 2;
			T* newPtr = (T*)realloc(writeBuf, sizeof(T) * writeCapacity);
			if (newPtr == nullptr) {
				Log::fatal("Could not extend write buffer to size: %d", writeCapacity);
			} else {
				Log::info("Extended write buffer to: %d", writeCapacity);
				writeBuf = newPtr;
			}
		}

		writeBuf[writeIndex++] = obj;
	}

	void swap() {
		lockRead();

		T* tmpPtr = readData;
		size_t tmpSize = readCapacity;

		readData = writeBuf;
		readCapacity = writeCapacity;
		readSize = writeIndex;

		writeBuf = tmpPtr;
		writeCapacity = tmpSize;
		writeIndex = 0;

		unlockRead();
	}

	AddableBuffer<T> getReadBuffer() {
		lockRead();
		AddableBuffer<T> b(readData, readSize, readCapacity);
		unlockRead();
		return b;
	}

	void lockRead() { readLock.lock(); }
	void unlockRead() { readLock.unlock(); }
};

template<typename T>
class AddableBuffer {
public:
	T* data;
	size_t index;
private:
	size_t capacity;
public:

	AddableBuffer(int initialCapacity) : capacity(initialCapacity), data((T*)malloc(initialCapacity * sizeof(T))), index(0) {
		if (data == nullptr) Log::fatal("Could not create AddableBuffer of size: %d", initialCapacity);
	}

	AddableBuffer(T* data, int dataSize, int initialCapacity) : capacity(initialCapacity), data((T*)malloc(initialCapacity * sizeof(T))), index(dataSize) {
		if (data == nullptr) Log::fatal("Could not create AddableBuffer of size: %d", initialCapacity);
		memcpy(this->data, data, dataSize * sizeof(T));
	}

	~AddableBuffer() {
		free(data);
	}

	AddableBuffer(const AddableBuffer&) = delete;
	AddableBuffer& operator=(const AddableBuffer&) = delete;
	AddableBuffer(AddableBuffer&& buf) : capacity(buf.capacity), index(buf.index), data(buf.data) { buf.data = nullptr; }
	AddableBuffer& operator=(AddableBuffer&& buf) {
		this->data = buf.data;
		this->index = buf.index;
		this->capacity = buf.capacity;
		buf.data = nullptr;
		return *this;
	}
	
	void add(const T& obj) {
		if (index == capacity) {
			capacity *= 2;
			T* newPtr = (T*)realloc(data, sizeof(T) * capacity);
			if (newPtr == nullptr) {
				Log::fatal("Could not extend write buffer to size: %d", capacity);
			} else {
				Log::info("Extended write buffer to: %d", capacity);
				data = newPtr;
			}
		}

		data[index++] = obj;
	}
};
