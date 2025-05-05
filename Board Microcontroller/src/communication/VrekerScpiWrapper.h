/**
 * @file VrekerSCPIWrapper.h
 * @brief Wrapper for the Vrekrer SCPI Parser library
 * @author Gabriel Avenia
 * @date May 2025
 *
 * @defgroup scpi SCPI Command Parser
 * @ingroup communication
 * @brief SCPI command parsing components
 * @{
 */

 #pragma once

 /**
  * @brief This wrapper prevents multiple inclusions of the SCPI parser implementation
  * 
  * The Vrekrer SCPI Parser library defines implementation in header files,
  * so we need to control its inclusion to prevent duplicate symbol errors.
  */
 #ifndef VREKRER_SCPI_IMPLEMENTATION
 #define VREKRER_SCPI_IMPLEMENTATION
 #include <Vrekrer_scpi_parser.h>
 #endif
 
 /** @} */ // End of scpi group