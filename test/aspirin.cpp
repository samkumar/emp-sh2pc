#include "emp-sh2pc/emp-sh2pc.h"
using namespace emp;
using namespace std;

struct input {
    Integer patient_id_concat_timestamp;
    Bit diagnosis; // or aspirin prescription
};

void aspirin(int input_size_per_party, int patient_id_bits = 32, int timestamp_bits = 32, int result_bits = 32) {
    int input_array_length = input_size_per_party * 2;
    Integer* patient_id_concat_timestamp = new Integer[input_array_length];
    Bit* diagnosis = new Bit[input_array_length]; // true if entry at this index is a diagnosis, false if it's an aspirin prescription
    struct input* inputs = new struct input[input_array_length];
    for (int i = 0; i != input_array_length; i++) {
        patient_id_concat_timestamp[i] = Integer(patient_id_bits + timestamp_bits, 0, i < input_size_per_party ? ALICE : ALICE);
        Integer b(1, 0, i < input_size_per_party ? ALICE : ALICE); // for now
        diagnosis[i] = b[0];
    }

    // Verify the input first.
    Bit verifyOrder(true);
    for (int i = 0; i < input_size_per_party - 1; i++) {
        Bit lessThanNext = !(patient_id_concat_timestamp[i] > patient_id_concat_timestamp[i+1]);
        verifyOrder = verifyOrder & lessThanNext;
    }
    for (int i = input_size_per_party; i < 2 * input_size_per_party - 1; i++) {
        Bit greaterThanNext = patient_id_concat_timestamp[i].geq(patient_id_concat_timestamp[i+1]);
        verifyOrder = verifyOrder & greaterThanNext;
    }
    verifyOrder.reveal<std::string>(ALICE);

    // Merge the two arrays, sorted ascending by patient_id_concat_timestamp
    bitonic_merge(patient_id_concat_timestamp, diagnosis, 0, input_array_length, true);

    Integer total(result_bits, 0, PUBLIC);
    Integer one(result_bits, 1, PUBLIC);

    // Now, for each input, check if it and the next input have the same patient, but the first is a diagnosis and the second isn't.
    for (int i = 0; i < input_array_length; i++) {
        Bit add = inputs[i].diagnosis & !inputs[i+1].diagnosis;
        add = add & ((inputs[i].patient_id_concat_timestamp >> timestamp_bits).equal(inputs[i+1].patient_id_concat_timestamp >> timestamp_bits));
        Integer next = total + one;
        total.select(add, next);
    }

    total.reveal<std::string>(ALICE);
    delete[] inputs;
}

int main(int argc, char** argv) {
    int size;
    if (argc == 3) {
        size = 128;
    } else if (argc == 4) {
        size = atoi(argv[3]);
    } else {
        cout << "Bad args" << endl;
        return 1;
    }

    int port, party;
    parse_party_and_port(argv, &party, &port);
    NetIO * io = new NetIO(party==ALICE ? nullptr : "127.0.0.1", port);

    setup_semi_honest(io, party);

    cout << "About to call aspirin" << endl;
    aspirin(size);
    delete io;
}
