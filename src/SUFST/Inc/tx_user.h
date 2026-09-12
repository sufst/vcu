/* Wrapper for Core/Inc/tx_user.h.
 *
 * FileX defines TX_DISABLE_ERROR_CHECKING internally (in each of its source
 * files) which conflicts with TX_SAFETY_CRITICAL. Clearing it here ensures
 * tx_api.h's safety-critical check passes. #include resolves to the real
 * tx_user.h in Core/Inc without recursion.
 */

#include "../../Core/Inc/tx_user.h"
#undef TX_DISABLE_ERROR_CHECKING
