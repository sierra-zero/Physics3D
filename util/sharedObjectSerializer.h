#pragma once

#include <map>
#include <vector>
#include <assert.h>
#include <limits>

#include "serializeBasicTypes.h"

template<typename T, typename SerializeID = uint32_t>
class SharedObjectSerializer {
	SerializeID curPredefinedID = std::numeric_limits<SerializeID>::max();
	SerializeID curDynamicID = 0;

	std::vector<T> itemsYetToSerialize;
public:
	std::map<T, SerializeID> objectToIDMap;

	SharedObjectSerializer() = default;
	template<typename ListType>
	SharedObjectSerializer(const ListType& knownObjects) {
		for(const T& obj : knownObjects) {
			addPredefined(obj);
		}
	}

	void addPredefined(const T& obj) {
		assert(objectToIDMap.find(obj) == objectToIDMap.end());
		objectToIDMap.emplace(obj, curPredefinedID);
		curPredefinedID--;
	}

	void include(const T& obj) {
		if(objectToIDMap.find(obj) == objectToIDMap.end()) {
			itemsYetToSerialize.push_back(obj);
			objectToIDMap.emplace(obj, curDynamicID);
			curDynamicID++;
		}
	}

	// The given deserializer must be of the form 'serialize(T, std::ostream&)'. The object may be passed by ref or const ref
	template<typename Serializer>
	void serializeRegistry(Serializer serialize, std::ostream& ostream) {
		::serialize<SerializeID>(static_cast<SerializeID>(itemsYetToSerialize.size()), ostream);
		for(const T& item : itemsYetToSerialize) {
			serialize(item, ostream);
		}
		itemsYetToSerialize.clear();
	}

	void serializeIDFor(const T& obj, std::ostream& ostream) const {
		auto found = objectToIDMap.find(obj);
		if(found == objectToIDMap.end()) throw SerializationException("The given object was not registered!");

		::serialize<SerializeID>((*found).second, ostream);
	}
};

template<typename T, typename SerializeID = uint32_t>
class SharedObjectDeserializer {
	SerializeID curPredefinedID = std::numeric_limits<SerializeID>::max();
	SerializeID curDynamicID = 0;

public:
	std::map<SerializeID, T> IDToObjectMap;

	SharedObjectDeserializer() = default;
	template<typename ListType>
	SharedObjectDeserializer(const ListType& knownObjects) {
		for(const T& obj : knownObjects) {
			addPredefined(obj);
		}
	}

	void addPredefined(const T& obj) {
		IDToObjectMap.emplace(curPredefinedID, obj);
		curPredefinedID--;
	}

	// The given deserializer must be of the form 'T deserialize(std::istream&)', it may return references or const refs. 
	template<typename Deserializer>
	void deserializeRegistry(Deserializer deserialize, std::istream& istream) {
		size_t numberOfObjectsToDeserialize = ::deserialize<SerializeID>(istream);
		for(size_t i = 0; i < numberOfObjectsToDeserialize; i++) {
			T object = deserialize(istream);
			IDToObjectMap.emplace(curDynamicID, object);
			curDynamicID++;
		}
	}

	T deserializeObject(std::istream& istream) const {
		SerializeID id = ::deserialize<SerializeID>(istream);

		auto found = IDToObjectMap.find(id);
		if(found == IDToObjectMap.end()) throw SerializationException("There is no associated object for the id " + std::to_string(id));

		return (*found).second;
	}
};


template<typename T, typename SerializeID = uint32_t>
class FixedSharedObjectSerializerDeserializer {
	std::map<T, SerializeID> objectToIDMap;
	std::map<SerializeID, T> IDToObjectMap;
	SerializeID currentID = 0;

public:
	void registerObject(const T& object) {
		objectToIDMap.emplace(object, currentID);
		IDToObjectMap.emplace(currentID, object);
		currentID++;
	}
	void removeObject(const T& object) {
		auto iter = objectToIDMap.find(object);
		SerializeID id = iter.second();
		IDToObjectMap.erase(id);
		objectToIDMap.erase(object);
	}
	FixedSharedObjectSerializerDeserializer(std::initializer_list<T> list) {
		for(const T& item : list) {
			registerObject(item);
		}
	}

	void serialize(const T& object, std::ostream& ostream) const {
		auto iter = objectToIDMap.find(object);
		if(iter != objectToIDMap.end()) {
			::serialize<SerializeID>((*iter).second, ostream);
		} else {
			throw SerializationException("The given object was not registered");
		}
	}
	T deserialize(std::istream& istream) const {
		SerializeID id = ::deserialize<SerializeID>(istream);
		auto iter = IDToObjectMap.find(id);
		if(iter != IDToObjectMap.end()) {
			return (*iter).second;
		} else {
			throw SerializationException("There is no registered object matching id " + std::to_string(id));
		}
	}
};
