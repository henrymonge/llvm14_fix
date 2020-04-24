#include "qdp.h"

#include <map>
#include <list>
#include <functional>

#include <iostream>
#include <fstream>

#include <type_traits>


namespace QDP
{

  typedef QDPPoolAllocator<QDPHIPAllocator> HIPPOOL;

  namespace
  {
    static HIPPOOL* __cache_pool_allocator;

    bool __cacheverbose = false;
    bool __launchverbose = false;

    HIPPOOL& get__cache_pool_allocator()
    {
      if (!__cache_pool_allocator) {
	__cache_pool_allocator = new HIPPOOL();
      }
      return *__cache_pool_allocator;
    }
  }


  std::vector<QDPCache::Entry>& QDPCache::get__vec_backed() {
    return __vec_backed;
  }


  bool qdp_cache_get_cache_verbose() { return __cacheverbose; }
  void qdp_cache_set_cache_verbose(bool b) {
    std::cout << "Set cache to verbose\n";
    __cacheverbose = b;
  }

  bool qdp_cache_get_launch_verbose() { return __launchverbose; }
  void qdp_cache_set_launch_verbose(bool b) {
    std::cout << "Set launch to verbose\n";
    __launchverbose = b;
  }


  void QDPCache::enableMemset(unsigned val)
  {
    get__cache_pool_allocator().enableMemset(val);
  }

  void QDPCache::setPoolSize(size_t s)
  {
    get__cache_pool_allocator().setPoolSize(s);
  }

  size_t QDPCache::getPoolSize()
  {
    return get__cache_pool_allocator().getPoolSize();
  }


  void QDPCache::suspend()
  {
    get__cache_pool_allocator().suspend();
  }


  void QDPCache::resume()
  {
    get__cache_pool_allocator().resume();
  }

  
  std::string QDPCache::stringStatus( Status s )
  {
    switch (s)
      {
      case Status::undef:
	return "-";
      case Status::host:
	return "host";
      case Status::device:
	return "device";
      default:
	return "unknown";
      }
  }

  
  void QDPCache::printInfo(int id)
  {
    if (id >= 0)
      {
	assert( vecEntry.size() > id );
	const Entry& e = vecEntry[id];
	printInfo(e);
      }
    else
      {
	QDPIO::cout << "id = -1 (NULL pointer)\n";
      }
  }

  
  void QDPCache::printInfo(const Entry& e)
  {
    QDPIO::cout << "id = " << e.Id
		<< ", size = " << e.size
		<< ", flags = ";
    
    if (e.flags & OwnHostMemory) QDPIO::cout << "own hst|";
    if (e.flags & OwnDeviceMemory) QDPIO::cout << "own dev|";
    if (e.flags & JitParam) QDPIO::cout << "param|";
    if (e.flags & Static) QDPIO::cout << "static|";
    if (e.flags & Multi) QDPIO::cout << "multi|";
    if (e.flags & Array) QDPIO::cout << "array|";
    QDPIO::cout << ", ";

    if (e.flags & Array) {
      QDPIO::cout << "elem_size = " << e.elem_size;
      QDPIO::cout << ", ";
    }

    if (e.status_vec.size() > 0)
      {
	QDPIO::cout << "status = ";
	for ( auto st : e.status_vec )
	  {
	    switch (st) {
	    case Status::undef:
	      QDPIO::cout << "-,";
	      break;
	    case Status::host:
	      QDPIO::cout << "host,";
	      break;
	    case Status::device:
	      QDPIO::cout << "device,";
	      break;
	    default:
	      QDPIO::cout << "unknown, \n";
	    }
	  }
      }

    if (e.flags & QDPCache::Flags::JitParam)
      {
	QDPIO::cout << "value = ";
	switch(e.param_type) {
	case JitParamType::float_: QDPIO::cout << (float)e.param.float_ << ", "; break;
	case JitParamType::double_: QDPIO::cout << (double)e.param.double_ << ", "; break;
	case JitParamType::int_: QDPIO::cout << (int)e.param.int_ << ", "; break;
	case JitParamType::int64_: QDPIO::cout << (int64_t)e.param.int64_ << ", "; break;
	case JitParamType::bool_:
	  if (e.param.bool_)
	    QDPIO::cout << "true, ";
	  else
	    QDPIO::cout << "false, ";
	  break;
	default:
	  QDPIO::cout << "(unkown jit param type)\n"; break;
	  assert(0);
	}
      }
    QDPIO::cout << "\n";
  }




  size_t QDPCache::getSize(int id) {
    assert( vecEntry.size() > id );
    const Entry& e = vecEntry[id];
    return e.size;
  }

    
  void QDPCache::growStack()
  {
    const int portion = 1024;
    vecEntry.resize( vecEntry.size() + portion );
    for ( int i = 0 ; i < portion ; i++ ) {
      stackFree.push( vecEntry.size()-i-1 );
    }
  }



  int QDPCache::registrateOwnHostMem( size_t size, const void* ptr , QDPCache::LayoutFptr func )
  {
    return add( size , Flags::OwnHostMemory , Status::host , ptr , NULL , func );
  }

  int QDPCache::registrate( size_t size, unsigned flags, QDPCache::LayoutFptr func )
  {
    return add( size , Flags::Empty , Status::undef , NULL , NULL , func );
  }
  

  int QDPCache::getNewId()
  {
    if (stackFree.size() == 0) {
      growStack();
    }

    int Id = stackFree.top();
    assert( vecEntry.size() > Id );

    stackFree.pop();

    return Id;
  }


  

  int QDPCache::addJitParamFloat(float i)
  {
    int Id = getNewId();
    Entry& e = vecEntry[ Id ];
    e.Id           = Id;
    e.size         = 0;
    e.flags        = Flags::JitParam;
    e.status_vec.clear();
    e.karg_vec.clear();
    e.param.float_ = i;
    e.param_type   = JitParamType::float_;
    e.multi.clear();
    e.iterTrack = lstTracker.insert( lstTracker.end() , Id );
    return Id;
  }

  int QDPCache::addJitParamDouble(double i)
  {
    int Id = getNewId();
    Entry& e = vecEntry[ Id ];
    e.Id            = Id;
    e.size         = 0;
    e.flags         = Flags::JitParam;
    e.status_vec.clear();
    e.karg_vec.clear();
    e.param.double_ = i;
    e.param_type   = JitParamType::double_;
    e.multi.clear();
    e.iterTrack = lstTracker.insert( lstTracker.end() , Id );
    return Id;
  }

  int QDPCache::addJitParamInt(int i)
  {
    int Id = getNewId();
    Entry& e = vecEntry[ Id ];
    e.Id           = Id;
    e.size         = 0;
    e.flags        = Flags::JitParam;
    e.status_vec.clear();
    e.karg_vec.clear();
    e.param.int_   = i;
    e.param_type   = JitParamType::int_;
    e.multi.clear();
    e.iterTrack = lstTracker.insert( lstTracker.end() , Id );
    return Id;
  }

  int QDPCache::addJitParamInt64(int64_t i)
  {
    int Id = getNewId();
    Entry& e = vecEntry[ Id ];
    e.Id            = Id;
    e.size         = 0;
    e.flags         = Flags::JitParam;
    e.status_vec.clear();
    e.karg_vec.clear();
    e.param.int64_  = i;
    e.param_type    = JitParamType::int64_;
    e.multi.clear();
    e.iterTrack = lstTracker.insert( lstTracker.end() , Id );
    return Id;
  }

  int QDPCache::addJitParamBool(bool i)
  {
    int Id = getNewId();
    Entry& e = vecEntry[ Id ];
    e.Id           = Id;
    e.size         = 0;
    e.flags        = Flags::JitParam;
    e.status_vec.clear();
    e.karg_vec.clear();
    e.param.bool_  = i;
    e.param_type   = JitParamType::bool_;
    e.multi.clear();
    e.iterTrack = lstTracker.insert( lstTracker.end() , Id );
    return Id;
  }

  
  int QDPCache::addMulti( const multi1d<QDPCache::ArgKey>& ids )
  {
    int Id = getNewId();
    Entry& e = vecEntry[ Id ];
    e.Id        = Id;
    e.flags     = Flags::Multi;
    e.status_vec.clear();
    e.karg_vec.clear();
    e.hstPtr    = NULL;
    e.devPtr    = NULL;
    e.fptr      = NULL;
    e.size      = ids.size() * sizeof(void*);
    
    e.multi.clear();
#if 1
    e.multi.resize(ids.size());
    for( int i = 0 ; i < ids.size() ; ++i )
      e.multi[i] = ids[i];
#else
    for( int i = 0 ; i < ids.size() ; ++i )
      e.multi.push_back( ids[i] );
#endif    

    e.iterTrack = lstTracker.insert( lstTracker.end() , Id );

    return Id;
  }

  namespace QDP_JIT_CACHE
  {
    std::map<void*,int> map_ptr_id;
  }

  
  void QDPCache::signoffViaPtr( void* ptr )
  {
    auto search = QDP_JIT_CACHE::map_ptr_id.find(ptr);
    if(search != QDP_JIT_CACHE::map_ptr_id.end()) {
      signoff( search->second );
      QDP_JIT_CACHE::map_ptr_id.erase(search);
    } else {
      QDPIO::cout << "QDP Cache: Ptr (sign off via ptr) not found\n";
      QDP_error_exit("giving up");
    }
  }


  int QDPCache::addDeviceStatic( size_t n_bytes )
  {
    void* dummy;
    return addDeviceStatic( &dummy, n_bytes );
  }

  int QDPCache::addDeviceStatic( void** ptr, size_t n_bytes , bool track_ptr )
  {
    int Id = getNewId();
    Entry& e = vecEntry[ Id ];

    while (!get__cache_pool_allocator().allocate( ptr , n_bytes )) {
      if (!spill_lru()) {
	QDP_error_exit("cache allocate_device_static: can't spill LRU object");
      }
    }

    e.Id        = Id;
    e.size      = n_bytes;
    e.flags     = Flags::Static;
    e.devPtr    = *ptr;
    e.multi.clear();
    e.status_vec.clear();
    e.karg_vec.clear();

    e.iterTrack = lstTracker.insert( lstTracker.end() , Id );

    if (track_ptr)
      {
	// Sanity: make sure the address is not already stored
	auto search = QDP_JIT_CACHE::map_ptr_id.find(e.devPtr);
	assert(search == QDP_JIT_CACHE::map_ptr_id.end());
	
	QDP_JIT_CACHE::map_ptr_id.insert( std::make_pair( e.devPtr , Id ) );
      }
    
    return Id;
  }


  int QDPCache::add( size_t size, Flags flags, Status status, const void* hstptr_, const void* devptr_, QDPCache::LayoutFptr func )
  {
    void * hstptr = const_cast<void*>(hstptr_);
    void * devptr = const_cast<void*>(devptr_);
    
    if (stackFree.size() == 0) {
      growStack();
    }

    int Id = stackFree.top();

    assert( vecEntry.size() > Id );
    Entry& e = vecEntry[ Id ];

    e.Id        = Id;
    e.size      = size;
    e.flags     = flags;
    //e.status    = status;
    e.hstPtr    = hstptr;
    e.devPtr    = devptr;
    e.fptr      = func;
    e.multi.clear();

    e.status_vec.clear();
    e.status_vec.resize(1,status);

    e.karg_vec.clear();
	
    e.iterTrack = lstTracker.insert( lstTracker.end() , Id );

    stackFree.pop();

    return Id;
  }


  
  int QDPCache::addArray( size_t element_size , int num_elements )
  {
    if (stackFree.size() == 0) {
      growStack();
    }

    int Id = stackFree.top();

    size_t size = element_size * num_elements;
    void* dev_ptr;
    void* hst_ptr;
    
    while (!get__cache_pool_allocator().allocate( &dev_ptr , size )) {
      if (!spill_lru()) {
	QDP_error_exit("cache allocate_device_static: can't spill LRU object");
      }
    }

    try {
      hst_ptr = (void*)QDP::Allocator::theQDPAllocator::Instance().allocate( size , QDP::Allocator::DEFAULT );
    }
    catch(std::bad_alloc) {
      QDP_error_exit("cache allocateHostMemory: host memory allocator flags=1 failed");
    }

    assert( vecEntry.size() > Id );
    Entry& e = vecEntry[ Id ];

    e.Id        = Id;
    e.size      = size;
    e.elem_size = element_size;
    e.flags     = QDPCache::Flags::Array;
    e.hstPtr    = hst_ptr;
    e.devPtr    = dev_ptr;
    e.fptr      = NULL;
    
    e.multi.clear();
    
    e.status_vec.clear();
    e.status_vec.resize( num_elements , Status::undef );

    e.karg_vec.clear();
    e.karg_vec.resize( num_elements , NULL );

    e.iterTrack = lstTracker.insert( lstTracker.end() , Id );

    stackFree.pop();

    //QDPIO::cout << "addArray id = " << Id << " flags = " << e.flags << "\n";
    
    return Id;
  }


  
  void QDPCache::zero_rep( int id )
  {
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];

    // For now, we support zero_rep only on arrays
    assert( e.flags & QDPCache::Flags::Array );

    // We only support, zero_rep on word type float/double/int (which are multiple of 4 bytes)
    assert( e.size % 4 == 0 );
    
    // Arrays always have legal dev/hst memory pointers and are never spilled
    HipMemset( e.devPtr , 0 , e.size/sizeof(unsigned) );
    
    e.status_vec.clear();
    e.status_vec.resize( e.size / e.elem_size , Status::device );
  }

  

  
  void QDPCache::signoff(int id) {
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];
    
    lstTracker.erase( e.iterTrack );

    if ( !( e.flags & ( Flags::JitParam | Flags::Static | Flags::Multi ) ) )
      {
	freeHostMemory( e );
      }
    
    if ( !( e.flags & ( Flags::JitParam ) ) )
      {
	freeDeviceMemory( e );
      }

    if ( e.flags & ( Flags::Multi | Flags::Array ) )
      {
	e.multi.clear();
	e.status_vec.clear();
	e.karg_vec.clear();
      }
    
    stackFree.push( id );
  }
  

  

  void QDPCache::assureOnHost(int id) {
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];
    assert(e.flags != Flags::JitParam);
    assert(e.flags != Flags::Static);
    assureHost( e );
  }

  void QDPCache::assureOnHost(int id, int elem_num) {
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];
    assert(e.flags != Flags::JitParam);
    assert(e.flags != Flags::Static);
    assureHost( e , elem_num );
  }


  void QDPCache::getHostPtr(void ** ptr , int id) {
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];

    assert(e.flags != Flags::JitParam);
    assert(e.flags != Flags::Static);

    assureHost( e );

    *ptr = e.hstPtr;
  }


  void* QDPCache::getHostArrayPtr( int id , int elem ) {
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];

    //QDPIO::cout << "getHostArrayPtr id = " << id << " flags = " << e.flags << " elem = " << elem << "\n";
	
    assert(e.flags != Flags::JitParam);
    assert(e.flags != Flags::Static);
    assert(e.flags & Flags::Array);

    assureOnHost( id , elem );

    return (void*)((size_t)e.hstPtr + e.elem_size * elem);
  }



  void QDPCache::freeHostMemory(Entry& e) {
    if ( e.flags & Flags::OwnHostMemory )
      return;
    
    if (!e.hstPtr)
      return;

    assert(e.flags != Flags::JitParam);
    assert(e.flags != Flags::Static);

    QDP::Allocator::theQDPAllocator::Instance().free( e.hstPtr );
    e.hstPtr=NULL;
  }


  
  void QDPCache::allocateHostMemory(Entry& e) {
    assert(e.flags != Flags::JitParam);
    assert(e.flags != Flags::Static);
    
    if ( e.flags & Flags::OwnHostMemory )
      return;
    
    if (e.hstPtr)
      return;
    
    try {
      e.hstPtr = (void*)QDP::Allocator::theQDPAllocator::Instance().allocate( e.size , QDP::Allocator::DEFAULT );
    }
    catch(std::bad_alloc) {
      QDP_error_exit("cache allocateHostMemory: host memory allocator flags=1 failed");
    }
  }


  void QDPCache::allocateDeviceMemory(Entry& e) {
    assert(e.flags != Flags::JitParam);
    assert(e.flags != Flags::Static);
    
    if ( e.flags & Flags::OwnDeviceMemory )
      return;

    if (e.devPtr)
      return;

    while (!get__cache_pool_allocator().allocate( &e.devPtr , e.size )) {
      if (!spill_lru()) {
	QDP_info("Device pool:");
	get__cache_pool_allocator().printListPool();
	//printLockSets();
	QDP_error_exit("cache assureDevice: can't spill LRU object. Out of GPU memory!");
      }
    }
  }



  void QDPCache::freeDeviceMemory(Entry& e) {
    //QDPIO::cout << "free size = " << e.size << "\n";
    assert(e.flags != Flags::JitParam);
    
    if ( e.flags & Flags::OwnDeviceMemory )
      return;

    if (!e.devPtr)
      return;

    get__cache_pool_allocator().free( e.devPtr );
    e.devPtr = NULL;
  }




  void QDPCache::copyD2H(int id) {
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];
    assert( e.flags & Flags::Array );

    if (qdp_cache_get_cache_verbose())
      {
	if (e.size >= 1024*1024)
	  QDPIO::cerr << "copy host <-- GPU " << e.size/1024/1024 << " MB\n";
	else
	  QDPIO::cerr << "copy host <-- GPU " << e.size << " bytes\n";
      }
    HipMemcpyD2H( e.hstPtr , e.devPtr , e.size );
    for( int i = 0 ; i < e.status_vec.size() ; ++i )
      e.status_vec[i] = Status::host;
  }


  

  void QDPCache::assureDevice(int id) {
    if (id < 0)
      return;
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];
    assureDevice(e);
  }

  void QDPCache::assureDevice(int id,int elem) {
    //QDPIO::cout << "assureDevice id=" << id << "  elem=" << elem << "\n";
    if (id < 0)
      return;
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];
    assureDevice(e,elem);
  }

  
  void QDPCache::assureDevice(Entry& e) {
    if (e.flags & Flags::JitParam)
      return;
    if (e.flags & Flags::Static)
      return;

    // new
    lstTracker.splice( lstTracker.end(), lstTracker , e.iterTrack );

    if (e.flags & Flags::Array)
      {
	//QDPIO::cout << "cache: making sure whole array is on device, size = " << e.status_vec.size() << "\n";
	assert( e.status_vec.size() == ( e.size / e.elem_size));
	for( int i = 0 ; i < e.status_vec.size() ; ++i )
	  {
	    assureDevice( e , i );
	  }
      }
    else
      {
	//std::cout << "id = " << e.Id << "  flags = " << e.flags << "\n";

	if (e.flags & Flags::Multi)
	  {
	    allocateDeviceMemory(e);
	  }
	else
	  {
	    assert( e.status_vec.size() == 1 );
	
	    if (e.status_vec[0] == Status::device)
	      return;
    
	    allocateDeviceMemory(e);

	    if ( e.status_vec[0] == Status::host )
	      {
		if (qdp_cache_get_cache_verbose())
		  {
		    if (e.size >= 1024*1024)
		      QDPIO::cerr << "copy host --> GPU " << e.size/1024/1024 << " MB\n";
		    else
		      QDPIO::cerr << "copy host --> GPU " << e.size << " bytes\n";
		  }

		if (e.fptr) {

		  char * tmp = new char[e.size];
		  e.fptr(true,tmp,e.hstPtr);
		  HipMemcpyH2D( e.devPtr , tmp , e.size );
		  delete[] tmp;

		} else {
		  HipMemcpyH2D( e.devPtr , e.hstPtr , e.size );
		}

	      }

	    e.status_vec[0] = Status::device;
	  }
      }
  }


  
  void QDPCache::assureDevice(Entry& e,int elem) {
    if (e.flags & Flags::JitParam)
      return;
    if (e.flags & Flags::Static)
      return;

    assert( e.flags & Flags::Array );
    assert( e.status_vec.size() > elem );
    
    // new
    lstTracker.splice( lstTracker.end(), lstTracker , e.iterTrack );

    if (e.status_vec[elem] == Status::device) {
      //std::cout << "already is on device\n";
      return;
    }
    
    //allocateDeviceMemory(e);

    if ( e.status_vec[elem] == Status::host )
      {
	assert( e.fptr == NULL );

	if (qdp_cache_get_cache_verbose())
	  {
	    QDPIO::cerr << "copy host --> GPU " << e.elem_size << " bytes\n";
	  }

	HipMemcpyH2D( (void*)((size_t)e.devPtr + e.elem_size * elem) ,
		       (void*)((size_t)e.hstPtr + e.elem_size * elem) , e.elem_size );
	//std::cout << "copy to device\n";
      }

    e.status_vec[elem] = Status::device;
  }

  



  void QDPCache::assureHost(Entry& e) {
    assert(e.flags != Flags::JitParam);
    assert(e.flags != Flags::Static);
    
    allocateHostMemory(e);

    if (e.flags & Flags::Array)
      {
	//QDPIO::cout << "cache: making sure whole array is on host\n";
	assert( e.status_vec.size() == ( e.size / e.elem_size));
	for( int i = 0 ; i < e.status_vec.size() ; ++i )
	  {
	    assureHost( e , i );
	  }
      }
    else
      {
	assert( e.status_vec.size() == 1 );

	if ( e.status_vec[0] == Status::device )
	  {
	    
	    if (qdp_cache_get_cache_verbose())
	      {
		if (e.size >= 1024*1024)
		  QDPIO::cerr << "copy host <-- GPU " << e.size/1024/1024 << " MB\n";
		else
		  QDPIO::cerr << "copy host <-- GPU " << e.size << " bytes\n";
	      }
	    
	    if (e.fptr) {
	      char * tmp = new char[e.size];
	      HipMemcpyD2H( tmp , e.devPtr , e.size );
	      e.fptr(false,e.hstPtr,tmp);
	      delete[] tmp;
	    } else {
	      HipMemcpyD2H( e.hstPtr , e.devPtr , e.size );
	    }
	  }

	e.status_vec[0] = Status::host;

	freeDeviceMemory(e);
      }
  }


  void QDPCache::assureHost( Entry& e, int elem_num ) {
    assert(e.flags != Flags::JitParam);
    assert(e.flags != Flags::Static);

    assert( e.flags & Flags::Array );
    assert(e.status_vec.size() > elem_num );

    // allocateHostMemory(e);

    if ( e.status_vec[elem_num] == Status::device )
      {
	assert( e.fptr == NULL );

	if (qdp_cache_get_cache_verbose())
	  {
	    QDPIO::cerr << "copy host <-- GPU " << e.elem_size << " bytes\n";
	  }

	HipMemcpyD2H( (void*)((size_t)e.hstPtr + e.elem_size * elem_num) ,
		       (void*)((size_t)e.devPtr + e.elem_size * elem_num) , e.elem_size );
      }

    //QDPIO::cout << "status was:" << stringStatus(e.status_vec[elem_num]) << "  new:" << stringStatus(Status::host) << "\n";
    e.status_vec[elem_num] = Status::host;
  }




  bool QDPCache::spill_lru() {
    if (lstTracker.size() < 1)
      return false;

    list<int>::iterator it_key = lstTracker.begin();
      
    bool found=false;
    Entry* e;

    while ( !found  &&  it_key != lstTracker.end() ) {
      e = &vecEntry[ *it_key ];

      found = ( (e->devPtr != NULL) &&
		(e->flags != Flags::JitParam) &&
		(e->flags != Flags::Static) &&
		(e->flags != Flags::Multi) &&
		(e->flags != Flags::Array) &&
		( ! (e->flags & Flags::OwnDeviceMemory) ) );
    
      if (!found)
	it_key++;
    }

    if (found) {
      //QDPIO::cout << "spill id = " << e->Id << "   size = " << e->size << "\n";
      assureHost( *e );
      return true;
    } else {
      return false;
    }
  }







  QDPCache::QDPCache() : vecEntry(1024)  {
    for ( int i = vecEntry.size()-1 ; i >= 0 ; --i ) {
      stackFree.push(i);
    }
  }


  bool QDPCache::isOnDevice(int id) {
    // An id < 0 indicates a NULL pointer
    if (id < 0)
      return true;
    
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];

    if (e.flags & QDPCache::JitParam)
      return true;

    if (e.flags & QDPCache::Static)
      return true;

    if (e.flags & QDPCache::Multi)
      {
	return e.devPtr != NULL;
      }

    if (e.flags & QDPCache::Array)
      {
	//QDPIO::cout << "making sure all elements of the array are on the device\n";
	bool ret = true;
	assert( e.status_vec.size() > 0 );
	for( int i = 0 ; i < e.status_vec.size() ; ++i )
	  ret = ret && (e.status_vec[i] == QDPCache::Status::device);
	return ret;
      }
    else
      {
	assert( e.status_vec.size() == 1 );
	return ( e.status_vec[0] == QDPCache::Status::device );
      }
  }


  bool QDPCache::isOnDevice(int id, int elem) {
    // An id < 0 indicates a NULL pointer
    if (id < 0)
      return true;
    
    assert( vecEntry.size() > id );
    Entry& e = vecEntry[id];

    if (e.flags & QDPCache::JitParam)
      return true;

    if (e.flags & QDPCache::Static)
      return true;

    assert(e.flags & QDPCache::Array);
    assert( e.status_vec.size() > elem );
    
    return ( e.status_vec[elem] == QDPCache::Status::device );
  }

  
  namespace {
    void* jit_param_null_ptr = NULL;
  }


  
  
  void QDPCache::backup_last_kernel_args()
  {
    for ( auto ptr : __hst_ptr ) {
      //QDPIO::cout << "free = " << (size_t)ptr << "\n";
      QDP::Allocator::theQDPAllocator::Instance().free( ptr );
    }
    __hst_ptr.clear();
    __vec_backed.clear();

    // All ids in 'ids_last' should be in 'cached' state
    // Now,
    // 1) allocate host memory
    // 2) back them up

    //QDPIO::cout << "backing up " << __ids_last.size() << " elements\n";

    for ( auto ak : __ids_last )
      {
	//QDPIO::cout << "backup: ";
	//id = " << ak.id << ", elem = " << ak.elem << "\n";
	//printInfo(ak.id);
	
	if (ak.id >= 0)
	  {
	    assert( vecEntry.size() > ak.id );
	    Entry& e = vecEntry[ak.id];

	    // We don't backup 'multis' for now
	    assert((e.flags & QDPCache::Flags::Multi) == 0);

	    if (e.flags & QDPCache::Flags::JitParam)
	      {
		__vec_backed.push_back( e );
		// no need to copy anything to host
	      }
	    else
	      {
		void* hst_ptr = (void*)QDP::Allocator::theQDPAllocator::Instance().allocate( e.size , QDP::Allocator::DEFAULT );

		HipMemcpyD2H( hst_ptr , e.devPtr , e.size );

		if (e.flags & QDPCache::Flags::Array)
		  {
		    // Use the param field to store the elem access field
		    // this covers both: whole array and element view since
		    // we backup the whole field
		    e.param.int_ = ak.elem;
		  }
		
		assert( e.Id == ak.id );
		__vec_backed.push_back( e );
		__vec_backed.back().hstPtr = hst_ptr;
	      }
	  }
	else
	  {
	    QDPCache::Entry null_entry;
	    null_entry.devPtr = NULL;
	    null_entry.Id = -1;
	    __vec_backed.push_back( null_entry );
	    // NULL id, no need to copy anything
	  }
	
      }
    //QDPIO::cout << "done backing up!\n";
  }


  namespace 
  {
    template<class T>
    void insert_ret( std::vector<unsigned char>& ret, T t )
    {
      if (std::is_pointer<T>::value)
	{
	  std::cout << "size is " << ret.size() << " resizing by " << ret.size() % sizeof(T) << " to meet padding requirements\n";
	  ret.resize( ret.size() + ret.size() % sizeof(T) );
	  ret.resize( ret.size() + sizeof(T) );
	  *(T*)&ret[ ret.size() - sizeof(T) ] = t;
	  std::cout << "inserted pointer (size " << sizeof(T) << ") " << t << "\n";
	}
      else
	{
	  ret.resize( ret.size() + sizeof(T) );
	  *(T*)&ret[ ret.size() - sizeof(T) ] = t;
	  std::cout << "inserted (size " << sizeof(T) << ") " << t << "\n";
	}
    }
    template<>
    void insert_ret<bool>( std::vector<unsigned char>& ret, bool t )
    {
      ret.resize( ret.size() + 4 );
      *(bool*)&ret[ ret.size() - 4 ] = t;
      std::cout << "inserted bool (as size 4) " << t << "\n";
    }

  } // namespace


  
  //std::vector<void*> QDPCache::get_kernel_args(std::vector<ArgKey>& ids , bool for_kernel )
  std::vector<unsigned char> QDPCache::get_kernel_args(std::vector<ArgKey>& ids , bool for_kernel )
  {
    if (qdp_cache_get_pool_bisect())
      {
	__ids_last = ids;
      }
    
    // Here we do two cycles through the ids:
    // 1) cache all objects
    // 2) check all are cached
    // This should replace the old 'lock set'

    //QDPIO::cout << "ids: ";
    std::vector<ArgKey> allids;
    for ( auto i : ids )
      {
	allids.push_back(i);
	if (i.id >= 0)
	  {
	    //QDPIO::cout << i << " ";
	    assert( vecEntry.size() > i.id );
	    Entry& e = vecEntry[i.id];
	    if (e.flags & QDPCache::Flags::Multi)
	      {
		for ( auto ak : e.multi )
		  {
		    allids.push_back( ak );
		  }
	      }
	  }
      }
    //QDPIO::cout << "\n";

    //QDPIO::cout << "assureDevice:\n";
    for ( auto i : allids ) {
      //QDPIO::cout << "id = " << i.id << ", elem = " << i.elem << "\n";
      if (i.elem < 0)
	assureDevice(i.id);
      else
	assureDevice(i.id,i.elem);
    }
    //QDPIO::cout << "done\n";
    
    bool all = true;
    for ( auto i : allids )
      if (i.elem < 0)
	all = all && isOnDevice(i.id);
      else
	all = all && isOnDevice(i.id,i.elem);

    
    if (!all) {
      QDPIO::cout << "It was not possible to load all objects required by the kernel into device memory\n";
      for ( auto i : ids ) {
	if (i.id >= 0)
	  {
	    assert( vecEntry.size() > i.id );
	    Entry& e = vecEntry[i.id];
	    QDPIO::cout << "id = " << i.id << "  size = " << e.size << "  flags = " << e.flags << "  status = ";
	    for ( auto st : e.status_vec )
	      {
		switch (st) {
		case Status::undef:
		  QDPIO::cout << "undef,";
		  break;
		case Status::host:
		  QDPIO::cout << "host,";
		  break;
		case Status::device:
		  QDPIO::cout << "device,";
		  break;
		default:
		  QDPIO::cout << "unkown\n";
		}
	      }
	  }
	else
	  {
	    QDPIO::cout << "id = " << i.id << "\n";
	  }
      }
      QDP_error_exit("giving up");
    }


    // Handle multi-ids
    for ( auto i : ids )
      {
	if (i.id >= 0)
	  {
	    Entry& e = vecEntry[i.id];
	    if (e.flags & QDPCache::Flags::Multi)
	      {
		assert( isOnDevice(i.id) );
		multi1d<void*> dev_ptr(e.multi.size());

		int count=0;
		for( auto ak : e.multi )  // q == id
		  {
		    if ( ak.id >= 0 )
		      {
			assert( vecEntry.size() > ak.id );
			Entry& qe = vecEntry[ ak.id ];
			assert( ! (qe.flags & QDPCache::Flags::Multi) );
			assert( isOnDevice( ak.id ) );
			if (ak.elem == -1)
			  {
			    dev_ptr[count++] = qe.devPtr;
			  }
			else
			  {
			    dev_ptr[count++] = (void*)((size_t)qe.devPtr + qe.elem_size * ak.elem);
			  }
		      }
		    else
		      {
			dev_ptr[count++] = NULL;
		      }
		  }
		
		if (qdp_cache_get_cache_verbose())
		  {
		    QDPIO::cerr << "copy host --> GPU " << e.multi.size() * sizeof(void*) << " bytes (kernel args, multi)\n";
		  }
		
		HipMemcpyH2D( e.devPtr , dev_ptr.slice() , e.multi.size() * sizeof(void*) );
		//QDPIO::cout << "multi-ids: copied elements = " << e.multi.size() << "\n";
	      }
	  }
      }

    
    const bool print_param = true;


#if 1
    if (print_param)
      QDPIO::cout << "Jit function param: ";
    
    std::vector<unsigned char> ret;

    for ( auto i : ids )
      {
	if (i.id >= 0)
	  {
	    Entry& e = vecEntry[i.id];
	    if (e.flags & QDPCache::Flags::JitParam)
	      {
		if (print_param)
		  {
		    switch(e.param_type) {
		    case JitParamType::float_: QDPIO::cout << (float)e.param.float_ << ", "; break;
		    case JitParamType::double_: QDPIO::cout << (double)e.param.double_ << ", "; break;
		    case JitParamType::int_: QDPIO::cout << (int)e.param.int_ << ", "; break;
		    case JitParamType::int64_: QDPIO::cout << (int64_t)e.param.int64_ << ", "; break;
		    case JitParamType::bool_:
		      if (e.param.bool_)
			QDPIO::cout << "true, ";
		      else
			QDPIO::cout << "false, ";
		      break;
		    default:
		      QDPIO::cout << "(unkown jit param type)\n"; break;
		      assert(0);
		    }
		  }
	  
		switch(e.param_type) {
		case JitParamType::float_: insert_ret<float>(ret, e.param.float_ ); break;
		case JitParamType::double_: insert_ret<double>(ret, e.param.double_ ); break;
		case JitParamType::int_: insert_ret<int>(ret, e.param.int_ );break;
		case JitParamType::int64_: insert_ret<int64_t>(ret, e.param.int64_ ); break;
		case JitParamType::bool_: insert_ret<bool>(ret, e.param.bool_ ); ;break;
		}
	      }
	    else
	      {
		if (print_param)
		  {
		    QDPIO::cout << (size_t)e.devPtr << ", ";
		  }

		if (e.flags & QDPCache::Flags::Array)
		  {
		    //assert(for_kernel);

		    if ( i.elem == -1 )
		      {
			// This ArgKey comes from an multiXd<OScalar> access, like in sumMulti
			//ret.push_back( for_kernel ? &e.devPtr : e.devPtr );
			insert_ret<void*>( ret , e.devPtr );
		      }
		    else
		      {
			// This ArgKey comes from an OScalar access through multiXd<OScalar> 
			assert( e.karg_vec.size() > i.elem );
			e.karg_vec[i.elem] = (void*)((size_t)e.devPtr + e.elem_size * i.elem);
			insert_ret<void*>( ret , e.karg_vec[i.elem] );
		      }
		  }
		else
		  {
		    insert_ret<void*>( ret , e.devPtr );
		  }
	      }
	  }
	else
	  {
	
	    if (print_param)
	      {
		QDPIO::cout << "NULL(id=" << i.id << "), ";
	      }

	    insert_ret<void*>( ret , &jit_param_null_ptr );
	
	  }
      }
    if (print_param)
      QDPIO::cout << "\n";
#else
    if (print_param)
      QDPIO::cout << "Jit function param: ";
    
    std::vector<void*> ret;
    for ( auto i : ids )
      {
	if (i.id >= 0)
	  {
	    Entry& e = vecEntry[i.id];
	    if (e.flags & QDPCache::Flags::JitParam)
	      {
		if (print_param)
		  {
		    switch(e.param_type) {
		    case JitParamType::float_: QDPIO::cout << (float)e.param.float_ << ", "; break;
		    case JitParamType::double_: QDPIO::cout << (double)e.param.double_ << ", "; break;
		    case JitParamType::int_: QDPIO::cout << (int)e.param.int_ << ", "; break;
		    case JitParamType::int64_: QDPIO::cout << (int64_t)e.param.int64_ << ", "; break;
		    case JitParamType::bool_:
		      if (e.param.bool_)
			QDPIO::cout << "true, ";
		      else
			QDPIO::cout << "false, ";
		      break;
		    default:
		      QDPIO::cout << "(unkown jit param type)\n"; break;
		      assert(0);
		    }
		  }
	  
		assert(for_kernel);
		ret.push_back( &e.param );
	  
	      }
	    else
	      {
		if (print_param)
		  {
		    QDPIO::cout << (size_t)e.devPtr << ", ";
		  }

		if (e.flags & QDPCache::Flags::Array)
		  {
		    //assert(for_kernel);

		    if ( i.elem == -1 )
		      {
			// This ArgKey comes from an multiXd<OScalar> access, like in sumMulti
			ret.push_back( for_kernel ? &e.devPtr : e.devPtr );
		      }
		    else
		      {
			// This ArgKey comes from an OScalar access through multiXd<OScalar> 
			assert( e.karg_vec.size() > i.elem );
			e.karg_vec[i.elem] = (void*)((size_t)e.devPtr + e.elem_size * i.elem);
			ret.push_back( for_kernel ? &e.karg_vec[i.elem] : e.karg_vec[i.elem] );
		      }
		  }
		else
		  {
		    ret.push_back( for_kernel ? &e.devPtr : e.devPtr );
		  }
	      }
	  }
	else
	  {
	
	    if (print_param)
	      {
		QDPIO::cout << "NULL(id=" << i.id << "), ";
	      }

	    assert(for_kernel);
	    ret.push_back( &jit_param_null_ptr );
	
	  }
      }
    if (print_param)
      QDPIO::cout << "\n";
#endif



    // std::cout << "get kernel args:\n";
    // std::cout << "p_ordered: " << (*(bool*)&ret[0]) << "\n";
    // std::cout << "p_th_count: " << (*(int*)&ret[1]) << "\n";
    // std::cout << "p_start: " << (*(int*)&ret[5]) << "\n";
    // std::cout << "p_end: " << (*(int*)&ret[9]) << "\n";
    // std::cout << "p_do_site_perm: " << (*(bool*)&ret[13]) << "\n";
    // std::cout << "p_site_table: " << (*(void**)&ret[14]) << "\n";
    // std::cout << "p_member_array: " << (*(void**)&ret[22]) << "\n";
    // std::cout << "ptr_dest: " << (*(void**)&ret[30]) << "\n";
    // std::cout << "ptr_a: " << (*(void**)&ret[38]) << "\n";
    // std::cout << "ptr_b: " << (*(void**)&ret[46]) << "\n";



    return ret;
  }

  namespace {
    static QDPCache* __global_cache;
  }

  QDPCache& QDP_get_global_cache()
  {
    if (!__global_cache) {
      __global_cache = new QDPCache();
    }
    return *__global_cache;
  }



  QDPCache::Flags operator|(QDPCache::Flags a, QDPCache::Flags b)
  {
    return static_cast<QDPCache::Flags>(static_cast<int>(a) | static_cast<int>(b));
  }



} // QDP



