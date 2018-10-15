# ResourcePackager

Resource serialize/deserialize support

## Concept

An serializable object can be serialized and deserialized. Its data is divided into readable configuration and unreadable binary data. Readable configuration is serialized into json object, and unreadable binary data is directly outputed (may add compression in the future).

An C++ class can be serialized/deserialized by inheriting from `xziar::respak::Serializable` and overridding corresponding method. 

An resource package `X` consist two files: `X.xzrp` and `X.xzrp.json`.

## `xzrp` File structure

xzrp file contains binary data. Each piece of data is kept in a block, identified by `sha256`.

An ResourceItem is used to record resource metadata. Its size is 64bytes. 

**ResourceItem Structure**
```
|=======|
|       |
|   S   |
|   H   |
|   A   |
|   2   |
|   5   |
|   6   |
|       |
|-------|
|  Size |
| uint64|
|-------|
| Offset|
| uint64|
|-------|
| Index |
|-------|
|       |
| Dummy |
|       |
|=======|
```
* `SHA256` is 32bytes sha256 hex
* `Size` is 8bytes LE-encoding for uint64, represent resource size
* `Offset` is 8bytes LE-encoding for uint64, represent resource's offset to file origin
* `Index` is 4bytes LE-encoding for uint32, represent resource's index in file (for check and lookup)
* `Dummy` is 12bytes dummy data, can be used for custom use 

**ResFile Structure**
```  
|------------|----| 
|ResourceItem|  64|  
| resource 1 | any|  
|------------|----|  
|   ......   |....|
|------------|----|
|ResourceItem|  64|  
| resource N | any|  
|------------|----|
|  res list  |64*N|  
|ResourceItem|  64|  
|------------|----|
```

The last block is the main block. The reslist contains every ResourceItem above in order and should be identical to them. 

The last `ResourceItem` treats res list as data and calculate sha256. `Offset` is reslist's offset in file and Size is ignored. `Index` is `N` so size can be calculated. `Dummy`'s first 4 bytes must be `xzrp`, so the file can be identified.

## `xzrp.json` File structure

The configuration file is the json file so can be modified easily.

**Object key begins with `#` is the internal field used by SerializeUtil**
* `#Type` represents object type, so the json-object can be deserilized by looking for its deserializer method.
* `#config` of the root object is used to identify resource file and its metadata.
* `#global` of the root object is used to store shared objects
* `#global_map` of the root object is used to store mapping relations of shared objects

There is no explicit relationship with `*.xzrp` file's resource. But generally, string `@***` represents a binary resource where `***` is sha256 of the resource.

There is also no explicit reference between objects. But generally, string `$***` represents a shared object where `***` is the key to find the object.

## Serializer

A Serializer is used to generate a pair of `*.xzrp` and `*.xzrp.json`. 

### Add filter

To correctly handle relative reference between objects, some object need to be put into specific region so they won't be serialized multiple times. `Filter` is used to generate corresponding path for the object.

`FilterFunc` is a function of `string(const string_view&)`, where argument is the SerializedType extract from the object. Generally the same type of object should be put into the same location.

Path being returned is a `JSON Pointer`, and it relies on [RapidJSON's JSON Pointer](http://rapidjson.org/zh-cn/md_doc_pointer_8zh-cn.html).

### Add object

To add an object, use `AddObject`.

`AddObject(JObjectLike target, string name, Serializable object)` is used to add an object to another object with provided key.

`AddObject(JArrayLike target, Serializable object)` is used to append an object to an array.

`AddObject(string name, JDoc node)` is used to add custom node to the root.

`AddObject(Serializable object, string id)` and `AddObject(JObject& object, string id)` are used to add an **shared** object to global space, where its path is determined by added filter. Its key is generated and returned so others can reference to it.

### Add resource
,...
To add a resource, use `PutResource(void* data, size_t size, string id)`.

`id` is used to quick check if resource has been added. Resources with the same id will be ignored even if their content is different. Empty id is ignored.

**If two resources' sha256 is identical, they are assumed to be identical.**

## Deserializer

A Deserializer is used to re-generate objects with given pair of `*.xzrp` and `*.xzrp.json`. 

### Use cookie

Some deserializer may need extra environment data to complete. So cookie can be set by deserialize host and be get by deserializer.

### Get object

`Deserialize<T>(JObjectRef object)` is used to generate an object from given node. `T` can be provided so the deserialized result will be checked and casted into proper type. Object is returned in `unique_ptr` so **it's always in heap to promise its polymorphism**.

**Type `T` is also used to lookup fallback deserializer.** If no corresponding deserializer registed, `T's DoDeserializer` (if it has) will be used to try to deserialize the object.

`DeserializeShare<T>(JObjectRef object, bool cache)` is similar to previous one, but convert `unique_ptr` to `shared_ptr`. Dur to its shareness, it allows to cache the generated object so an object won't be deserialized twice.

`Deserialize<T>(string_view id)` and `DeserializeShare<T>(string_view id, bool cache)` are used to deserialize shared object, which has similar effect with above.

### Get resource

`AlignedBuffer GetResource(string_view& handle, bool cache)` is used to get resource. Since it returns an `AlignedBuffer`, it naturally support cache mechanism. **Remember that AlignedBuffer is not COW, so any in-place-writes to it will influence the cached data too.**

## Serializable

Serializable provide serialize and deserialize support for the object.

`string_view SerializedType()` is needed to be overrided so object's actual type can be returned. It is used to lookup deserializer.

`void Serialize(SerializeUtil&, JObject&)` is optional to be overrided so an object can be correctly serialized.

`void Deserialize(DeserializeUtil&, JObjectRef&)` is also optional and it only deserialized data to an existing object.

`static unique_ptr<Serializable> DoDeserialize(DeserializeUtil&, JObjectRef&)` is used to perform actual deserialization so it's a public static function.

`static std::any DeserializeArg(DeserializeUtil&, JObjectRef&)` is used to provide constructor arguments when construct an object. Arguments are parsed and packed into `tuple`, then stored in `std::any`.

`RESPAK_DECL_SIMP_DESERIALIZE(typestr)` and `RESPAK_IMPL_SIMP_DESERIALIZE(type)` is used to quick declare & implement deserialize support. It requires a default-constructor.

`RESPAK_DECL_COMP_DESERIALIZE(typestr)` and `RESPAK_IMPL_COMP_DESERIALIZE(type, ...)` is used to quick declare & implement deserialize support. It is used when no default-constructor can be found. Constructor argument should be explicit listed at the end of impl-macro, and **a function-body should be follow by the impl-macro, which returns an `std::any` containing the `tuple`**.

## License

miniLogger (including its component) is licensed under the [MIT license](../../License.txt).
