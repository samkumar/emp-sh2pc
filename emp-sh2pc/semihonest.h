#ifndef SEMIHONEST_H__
#define SEMIHONEST_H__
#include "emp-sh2pc/semihonest_gen.h"
#include "emp-sh2pc/semihonest_eva.h"

namespace emp {
template<typename IO>
inline void setup_semi_honest(IO* io, int party) {
	if(party == ALICE) {
		HalfGateGen<IO, off> * t = new HalfGateGen<IO, off>(io);
		CircuitExecution::circ_exec = t;
		ProtocolExecution::prot_exec = new SemiHonestGen<IO>(io, t);
	} else {
		HalfGateEva<IO, off> * t = new HalfGateEva<IO, off>(io);
		CircuitExecution::circ_exec = t;
		ProtocolExecution::prot_exec = new SemiHonestEva<IO>(io, t);
	}
}
}
#endif
