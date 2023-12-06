/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2017 - 2019) :
 *
 * Alexandre Honorat <alexandre.honorat@insa-rennes.fr> (2019)
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2017 - 2019)
 * Julien Hascoet <jhascoet@kalray.eu> (2017)
 *
 * This software is a computer program whose purpose is to help prototyping
 * parallel applications using dataflow formalism.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
/*
 ============================================================================
 Name        : dump.c
 Author      : farresti, ahonorat, kdesnos, jhascoet
 Version     : 1.0
 Copyright   : CECILL-C
 Description : Function called by code generated by the Instrumented C
 Printer of Preesm
 ============================================================================
 */

#include "preesm_gen.h"

#include "dump.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

static double getElapsedMicroSec(uint64_t *start, uint64_t *end) {
    return getElapsedNanoSec(start, end) / ((double) 1e3);
}

static double getElapsedMilliSec(uint64_t *start, uint64_t *end) {
    return getElapsedNanoSec(start, end) / ((double) 1e6);
}

static double getElapsedSec(uint64_t *start, uint64_t *end) {
    return getElapsedNanoSec(start, end) / ((double) 1e9);
}

#else

static inline double getElapsedMicroSec(uint64_t *start, uint64_t *end) {
  return getElapsedNanoSec(start, end) / ((double) 1e3);
}

static inline double getElapsedMilliSec(uint64_t *start, uint64_t *end) {
  return getElapsedNanoSec(start, end) / ((double) 1e6);
}

static inline double getElapsedSec(uint64_t *start, uint64_t *end) {
  return getElapsedNanoSec(start, end) / ((double) 1e9);
}

#endif

static FILE *ptfile;

void initNbExec(int *nbExec, int nbDump) {
  int i = 0;

  if ((ptfile = fopen(DUMP_FILE, "a+")) == NULL) {
    fprintf(stderr, "ERROR: Cannot open dump file '%s'\n", DUMP_FILE);
    exit(1);
  }

  // Go to the end:
  fseek(ptfile, 0, SEEK_END);

  //printf(";;");
  for (i = 0; i < nbDump / 2; i++) {
    nbExec[i] = 1;
    fprintf(ptfile, "%d;", i);
  }
  fprintf(ptfile, "\n");
  fflush(ptfile);
}

void writeTime(uint64_t *dumpBuffer, int nbDump, int *nbExec) {

#if defined(DEVICE_C6678) || defined(SOC_C6678)
    CACHE_invL2(dumpBuffer, nbDump*sizeof(uint64_t), CACHE_WAIT);
#endif

  static int stable = 0;
  int i;
  int nbNotReady = 0;

  if (stable == 0) {
    for (i = 0; i < nbDump / 2; i++) {

      double time = getElapsedNanoSec(dumpBuffer + i * 2, dumpBuffer + i * 2 + 1);
      // We consider that all measures below MIN_TIME_MEASURE nanoseconds are not precise enough
      int nbExecBefore = nbExec[i];
      if (time < (double) 0) {
        nbExec[i] = 0;
        fprintf(stderr, "Error taking timing %d\n.", i);
      } else if (time < (double) (MIN_TIME_MEASURE)) {
        nbExec[i] = nbExecBefore * 2;
        if (nbExec[i] > MAX_ITER_MEASURE) {
          nbExec[i] = MAX_ITER_MEASURE;
        }
      }
      if (nbExecBefore != nbExec[i]) {
        nbNotReady++;
      }
    }
    if (nbNotReady == 0) {
      stable = 1;
    }
  }
  fprintf(stderr, "Ready: %d/%d\n", nbDump / 2 - nbNotReady, nbDump / 2);

  for (i = 0; i < nbDump / 2; i++) {
    //fprintf(stderr, "nbExec[%d]: %d\n", i, nbExec[i]);
    int nbEx = nbExec[i];
    double res;
    if (nbExec == 0) {
      res = 0.0;
    } else {
      res = getElapsedNanoSec(dumpBuffer + i * 2, dumpBuffer + i * 2 + 1) / nbEx;
    }
    fprintf(ptfile, "%.0lf;", res);
  }
  fprintf(ptfile, "\n");
  fflush(ptfile);

}