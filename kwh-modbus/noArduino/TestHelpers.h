#pragma once
#ifdef TEST
#define protected_testable public
#define private_testable public
#else
#define protected_testable protected
#define private_testable private
#endif