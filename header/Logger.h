/**
 * Autogenerated by Thrift Compiler (0.14.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef Logger_H
#define Logger_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include <memory>
#include "logger_types.h"



#ifdef _MSC_VER
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class LoggerIf {
 public:
  virtual ~LoggerIf() {}
  virtual int32_t AddLog(const std::string& level, const std::string& module_name, const std::string& message) = 0;
  virtual int32_t orisolLog(const std::string& level, const std::string& module_name, const std::string& message) = 0;
};

class LoggerIfFactory {
 public:
  typedef LoggerIf Handler;

  virtual ~LoggerIfFactory() {}

  virtual LoggerIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(LoggerIf* /* handler */) = 0;
};

class LoggerIfSingletonFactory : virtual public LoggerIfFactory {
 public:
  LoggerIfSingletonFactory(const ::std::shared_ptr<LoggerIf>& iface) : iface_(iface) {}
  virtual ~LoggerIfSingletonFactory() {}

  virtual LoggerIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(LoggerIf* /* handler */) {}

 protected:
  ::std::shared_ptr<LoggerIf> iface_;
};

class LoggerNull : virtual public LoggerIf {
 public:
  virtual ~LoggerNull() {}
  int32_t AddLog(const std::string& /* level */, const std::string& /* module_name */, const std::string& /* message */) {
    int32_t _return = 0;
    return _return;
  }
  int32_t orisolLog(const std::string& /* level */, const std::string& /* module_name */, const std::string& /* message */) {
    int32_t _return = 0;
    return _return;
  }
};

typedef struct _Logger_AddLog_args__isset {
  _Logger_AddLog_args__isset() : level(false), module_name(false), message(false) {}
  bool level :1;
  bool module_name :1;
  bool message :1;
} _Logger_AddLog_args__isset;

class Logger_AddLog_args {
 public:

  Logger_AddLog_args(const Logger_AddLog_args&);
  Logger_AddLog_args& operator=(const Logger_AddLog_args&);
  Logger_AddLog_args() : level(), module_name(), message() {
  }

  virtual ~Logger_AddLog_args() noexcept;
  std::string level;
  std::string module_name;
  std::string message;

  _Logger_AddLog_args__isset __isset;

  void __set_level(const std::string& val);

  void __set_module_name(const std::string& val);

  void __set_message(const std::string& val);

  bool operator == (const Logger_AddLog_args & rhs) const
  {
    if (!(level == rhs.level))
      return false;
    if (!(module_name == rhs.module_name))
      return false;
    if (!(message == rhs.message))
      return false;
    return true;
  }
  bool operator != (const Logger_AddLog_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Logger_AddLog_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Logger_AddLog_pargs {
 public:


  virtual ~Logger_AddLog_pargs() noexcept;
  const std::string* level;
  const std::string* module_name;
  const std::string* message;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Logger_AddLog_result__isset {
  _Logger_AddLog_result__isset() : success(false) {}
  bool success :1;
} _Logger_AddLog_result__isset;

class Logger_AddLog_result {
 public:

  Logger_AddLog_result(const Logger_AddLog_result&);
  Logger_AddLog_result& operator=(const Logger_AddLog_result&);
  Logger_AddLog_result() : success(0) {
  }

  virtual ~Logger_AddLog_result() noexcept;
  int32_t success;

  _Logger_AddLog_result__isset __isset;

  void __set_success(const int32_t val);

  bool operator == (const Logger_AddLog_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Logger_AddLog_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Logger_AddLog_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Logger_AddLog_presult__isset {
  _Logger_AddLog_presult__isset() : success(false) {}
  bool success :1;
} _Logger_AddLog_presult__isset;

class Logger_AddLog_presult {
 public:


  virtual ~Logger_AddLog_presult() noexcept;
  int32_t* success;

  _Logger_AddLog_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _Logger_orisolLog_args__isset {
  _Logger_orisolLog_args__isset() : level(false), module_name(false), message(false) {}
  bool level :1;
  bool module_name :1;
  bool message :1;
} _Logger_orisolLog_args__isset;

class Logger_orisolLog_args {
 public:

  Logger_orisolLog_args(const Logger_orisolLog_args&);
  Logger_orisolLog_args& operator=(const Logger_orisolLog_args&);
  Logger_orisolLog_args() : level(), module_name(), message() {
  }

  virtual ~Logger_orisolLog_args() noexcept;
  std::string level;
  std::string module_name;
  std::string message;

  _Logger_orisolLog_args__isset __isset;

  void __set_level(const std::string& val);

  void __set_module_name(const std::string& val);

  void __set_message(const std::string& val);

  bool operator == (const Logger_orisolLog_args & rhs) const
  {
    if (!(level == rhs.level))
      return false;
    if (!(module_name == rhs.module_name))
      return false;
    if (!(message == rhs.message))
      return false;
    return true;
  }
  bool operator != (const Logger_orisolLog_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Logger_orisolLog_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Logger_orisolLog_pargs {
 public:


  virtual ~Logger_orisolLog_pargs() noexcept;
  const std::string* level;
  const std::string* module_name;
  const std::string* message;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Logger_orisolLog_result__isset {
  _Logger_orisolLog_result__isset() : success(false) {}
  bool success :1;
} _Logger_orisolLog_result__isset;

class Logger_orisolLog_result {
 public:

  Logger_orisolLog_result(const Logger_orisolLog_result&);
  Logger_orisolLog_result& operator=(const Logger_orisolLog_result&);
  Logger_orisolLog_result() : success(0) {
  }

  virtual ~Logger_orisolLog_result() noexcept;
  int32_t success;

  _Logger_orisolLog_result__isset __isset;

  void __set_success(const int32_t val);

  bool operator == (const Logger_orisolLog_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Logger_orisolLog_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Logger_orisolLog_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Logger_orisolLog_presult__isset {
  _Logger_orisolLog_presult__isset() : success(false) {}
  bool success :1;
} _Logger_orisolLog_presult__isset;

class Logger_orisolLog_presult {
 public:


  virtual ~Logger_orisolLog_presult() noexcept;
  int32_t* success;

  _Logger_orisolLog_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class LoggerClient : virtual public LoggerIf {
 public:
  LoggerClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
    setProtocol(prot);
  }
  LoggerClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  int32_t AddLog(const std::string& level, const std::string& module_name, const std::string& message);
  void send_AddLog(const std::string& level, const std::string& module_name, const std::string& message);
  int32_t recv_AddLog();
  int32_t orisolLog(const std::string& level, const std::string& module_name, const std::string& message);
  void send_orisolLog(const std::string& level, const std::string& module_name, const std::string& message);
  int32_t recv_orisolLog();
 protected:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
};

class LoggerProcessor : public ::apache::thrift::TDispatchProcessor {
 protected:
  ::std::shared_ptr<LoggerIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (LoggerProcessor::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef std::map<std::string, ProcessFunction> ProcessMap;
  ProcessMap processMap_;
  void process_AddLog(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_orisolLog(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  LoggerProcessor(::std::shared_ptr<LoggerIf> iface) :
    iface_(iface) {
    processMap_["AddLog"] = &LoggerProcessor::process_AddLog;
    processMap_["orisolLog"] = &LoggerProcessor::process_orisolLog;
  }

  virtual ~LoggerProcessor() {}
};

class LoggerProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  LoggerProcessorFactory(const ::std::shared_ptr< LoggerIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::std::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::std::shared_ptr< LoggerIfFactory > handlerFactory_;
};

class LoggerMultiface : virtual public LoggerIf {
 public:
  LoggerMultiface(std::vector<std::shared_ptr<LoggerIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~LoggerMultiface() {}
 protected:
  std::vector<std::shared_ptr<LoggerIf> > ifaces_;
  LoggerMultiface() {}
  void add(::std::shared_ptr<LoggerIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  int32_t AddLog(const std::string& level, const std::string& module_name, const std::string& message) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->AddLog(level, module_name, message);
    }
    return ifaces_[i]->AddLog(level, module_name, message);
  }

  int32_t orisolLog(const std::string& level, const std::string& module_name, const std::string& message) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->orisolLog(level, module_name, message);
    }
    return ifaces_[i]->orisolLog(level, module_name, message);
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
class LoggerConcurrentClient : virtual public LoggerIf {
 public:
  LoggerConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot, std::shared_ptr<::apache::thrift::async::TConcurrentClientSyncInfo> sync) : sync_(sync)
{
    setProtocol(prot);
  }
  LoggerConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot, std::shared_ptr<::apache::thrift::async::TConcurrentClientSyncInfo> sync) : sync_(sync)
{
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  int32_t AddLog(const std::string& level, const std::string& module_name, const std::string& message);
  int32_t send_AddLog(const std::string& level, const std::string& module_name, const std::string& message);
  int32_t recv_AddLog(const int32_t seqid);
  int32_t orisolLog(const std::string& level, const std::string& module_name, const std::string& message);
  int32_t send_orisolLog(const std::string& level, const std::string& module_name, const std::string& message);
  int32_t recv_orisolLog(const int32_t seqid);
 protected:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
  std::shared_ptr<::apache::thrift::async::TConcurrentClientSyncInfo> sync_;
};

#ifdef _MSC_VER
  #pragma warning( pop )
#endif



#endif
