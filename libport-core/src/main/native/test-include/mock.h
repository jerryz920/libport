
#ifndef _JUTILS_MOCK_H
#define _JUTILS_MOCK_H

#include <list>
#include <utility>

#define CONCAT(x, y) x ## y
#define CALL_COUNT(name) name ## _call_count
#define RETURN_VALUE(name) name ## _return_value
#define ARG_TYPE(name) name ## _arg_type
#define CALL_ARGS(name) name ## _call_args

/// TODO: add things here... Question: can we move to GoogleMock or use
// variadic template for all the stuff...
#define DECL_ARGS0(name)

#define DECL_ARGS1(name, t1)

#define DECL_ARGS2(name, t1, t2)

#define DECL_ARGS3(name, t1, t2, t3)

#define DECL_ARGS4(name, t1, t2, t3, t4)

#define DECL_ARGS5(name, t1, t2, t3, t4, t5)

#define DECL_ARGS6(name, t1, t2, t3, t4, t5, t6)

#define MOCK_METHOD0(ret, name, ...)\
  int64_t CALL_COUNT(name) = 0; \
  ret RETURN_VALUE(name); \
  ret name() __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    return RETURN_VALUE(name); \
  }

#define MOCK_VOID_METHOD0(name, ...)\
  int64_t CALL_COUNT(name) = 0; \
  void name() __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
  }

#define MOCK_METHOD1(ret, name, t1, a1, ...) \
  int64_t CALL_COUNT(name) = 0; \
  ret RETURN_VALUE(name); \
  typedef std::tuple< \
      std::remove_reference<t1>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  ret name(t1 a1) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1)); \
    return RETURN_VALUE(name); \
  }

#define MOCK_VOID_METHOD1(name, t1, a1, ...) \
  int64_t CALL_COUNT(name) = 0; \
  typedef std::tuple< \
      std::remove_reference<t1>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  void name(t1 a1) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1)); \
  }

#define MOCK_METHOD2(ret, name, t1, a1, t2, a2, ...) \
  int64_t CALL_COUNT(name) = 0; \
  ret RETURN_VALUE(name); \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  ret name(t1 a1, t2 a2) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2)); \
    return RETURN_VALUE(name); \
  }

#define MOCK_VOID_METHOD2(name, t1, a1, t2, a2, ...) \
  int64_t CALL_COUNT(name) = 0; \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  void name(t1 a1, t2 a2) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2)); \
  }

#define MOCK_METHOD3(ret, name, t1, a1, t2, a2, t3, a3, ...)\
  int64_t CALL_COUNT(name) = 0; \
  ret RETURN_VALUE(name); \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type, \
      std::remove_reference<t3>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  ret name(t1 a1, t2 a2, t3 a3) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2, a3)); \
    return RETURN_VALUE(name); \
  }

#define MOCK_VOID_METHOD3(name, t1, a1, t2, a2, t3, a3, ...)\
  int64_t CALL_COUNT(name) = 0; \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type, \
      std::remove_reference<t3>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  void name(t1 a1, t2 a2, t3 a3) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2, a3)); \
  }

#define MOCK_METHOD4(ret, name, t1, a1, t2, a2, t3, a3, t4, a4, ...) \
  int64_t CALL_COUNT(name) = 0; \
  ret RETURN_VALUE(name); \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type, \
      std::remove_reference<t3>::type, \
      std::remove_reference<t4>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  ret name(t1 a1, t2 a2, t3 a3, t4 a4) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2, a3, a4)); \
    return RETURN_VALUE(name); \
  }

#define MOCK_VOID_METHOD4(name, t1, a1, t2, a2, t3, a3, t4, a4, ...) \
  int64_t CALL_COUNT(name) = 0; \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type, \
      std::remove_reference<t3>::type, \
      std::remove_reference<t4>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  void name(t1 a1, t2 a2, t3 a3, t4 a4) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2, a3, a4)); \
  }

#define MOCK_METHOD5(ret, name, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, ...) \
  int64_t CALL_COUNT(name) = 0; \
  ret RETURN_VALUE(name); \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type, \
      std::remove_reference<t3>::type, \
      std::remove_reference<t4>::type, \
      std::remove_reference<t5>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  ret name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2, a3, a4, a5)); \
    return RETURN_VALUE(name); \
  }

#define MOCK_VOID_METHOD5(name, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, ...) \
  int64_t CALL_COUNT(name) = 0; \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type, \
      std::remove_reference<t3>::type, \
      std::remove_reference<t4>::type, \
      std::remove_reference<t5>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  void name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2, a3, a4, a5)); \
  }

#define MOCK_METHOD6(ret, name, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6, ...) \
  int64_t CALL_COUNT(name) = 0; \
  ret RETURN_VALUE(name); \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type, \
      std::remove_reference<t3>::type, \
      std::remove_reference<t4>::type, \
      std::remove_reference<t5>::type, \
      std::remove_reference<t6>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  ret name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2, a3, a4, a5, a6)); \
    return RETURN_VALUE(name); \
  }


#define MOCK_VOID_METHOD6(name, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6, ...) \
  int64_t CALL_COUNT(name) = 0; \
  typedef std::tuple< \
      std::remove_reference<t1>::type, \
      std::remove_reference<t2>::type, \
      std::remove_reference<t3>::type, \
      std::remove_reference<t4>::type, \
      std::remove_reference<t5>::type, \
      std::remove_reference<t6>::type> \
      ARG_TYPE(name); \
  std::list<ARG_TYPE(name)> CALL_ARGS(name); \
  void name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) __VA_ARGS__ {\
    CALL_COUNT(name) += 1; \
    CALL_ARGS(name).push_back(ARG_TYPE(name)(a1, a2, a3, a4, a5, a6)); \
  }





#endif
