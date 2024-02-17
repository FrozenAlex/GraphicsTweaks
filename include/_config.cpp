#pragma once

#define GT_EXPORT __attribute__((visibility("default")))
#ifdef __cplusplus
#define GT_EXPORT_FUNC extern "C" GT_EXPORT
#else
#define GT_EXPORT_FUNC GT_EXPORT
#endif
