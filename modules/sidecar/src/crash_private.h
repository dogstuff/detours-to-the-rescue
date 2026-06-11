#ifndef DTTR_SIDECAR_CRASH_PRIVATE_H
#define DTTR_SIDECAR_CRASH_PRIVATE_H

#include <stdbool.h>

#include <dttr_mods.h>
#include <dttr_runtime.h>

// Registers and clears the PCDogs symbol provider used by crash dumps.
void dttr_pcdogs_crash_symbols_register(const DTTR_Core_Context *runtime);
void dttr_pcdogs_crash_symbols_clear();

// Writes a crash dump and stack trace for the DTTR_Mods_API exception report entry.
bool dttr_sidecar_write_exception_report(
	const DTTR_Mods_ExceptionReportRequest *request,
	DTTR_Mods_ExceptionReport *report
);

#endif // DTTR_SIDECAR_CRASH_PRIVATE_H
