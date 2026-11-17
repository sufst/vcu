/******************************************************************************
 * @file    status.h
 * @author  Tim Brewis (@t-bre)
 * @brief   Generic status return code
 *****************************************************************************/

#ifndef STATUS_H
#define STATUS_H

typedef enum
{
    STATUS_OK,
    STATUS_ERROR
} status_t;

typedef enum
{
    STATUS_THRESHOLD_OK,
    STATUS_THRESHOLD_MISMATCH,
    STATUS_THRESHOLD_WARNING,
    STATUS_THRESHOLD_ERROR
} scs_status_t;

#endif