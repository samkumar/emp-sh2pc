#include <chrono>
#include "emp-sh2pc/emp-sh2pc.h"
using namespace emp;
using namespace std;

struct input {
    Integer patient_id_concat_timestamp;
    Bit diagnosis; // or aspirin prescription
};

void aspirin(uint64_t input_size_per_party, int patient_id_bits = 32, int timestamp_bits = 32, int result_bits = 32) {
    uint64_t input_array_length = input_size_per_party * 2;
    Integer* patient_id_concat_timestamp = new Integer[input_array_length];
    Bit* diagnosis = new Bit[input_array_length]; // true if entry at this index is a diagnosis, false if it's an aspirin prescription
    for (uint64_t i = 0; i != input_array_length; i++) {
        patient_id_concat_timestamp[i] = Integer(patient_id_bits + timestamp_bits, i < input_size_per_party ? (((input_size_per_party - i - 1) << 32) | 1) : (((i - input_size_per_party) << 32) | 2), i < input_size_per_party ? ALICE : ALICE);
        Integer b(1, (i < input_size_per_party - 1) ? 1 : 0, i < input_size_per_party ? ALICE : ALICE); // for now
        diagnosis[i] = b[0];
    }

    // Verify the input first.
    Bit verifyOrder(true);
    for (uint64_t i = 0; i < input_size_per_party - 1; i++) {
        Bit lessThanNext = patient_id_concat_timestamp[i].geq(patient_id_concat_timestamp[i+1]);
        verifyOrder = verifyOrder & lessThanNext;
    }
    for (uint64_t i = input_size_per_party; i < 2 * input_size_per_party - 1; i++) {
        Bit greaterThanNext = !(patient_id_concat_timestamp[i] > patient_id_concat_timestamp[i+1]);
        verifyOrder = verifyOrder & greaterThanNext;
    }
    bool sorted = verifyOrder.reveal<bool>(ALICE);

    // Merge the two arrays, sorted ascending by patient_id_concat_timestamp
    bitonic_merge(patient_id_concat_timestamp, diagnosis, 0, input_array_length, true);

    Integer total(result_bits, 0, PUBLIC);
    Integer one(result_bits, 1, PUBLIC);

    // Now, for each input, check if it and the next input have the same patient, but the first is a diagnosis and the second isn't.
    for (uint64_t i = 0; i < input_array_length - 1; i++) {
        Bit add = diagnosis[i] & !diagnosis[i + 1];
        add = add & ((patient_id_concat_timestamp[i] >> timestamp_bits).equal(patient_id_concat_timestamp[i + 1] >> timestamp_bits));
        Integer next = total + one;
        total = total.select(add, next);
    }

    std::uint32_t result = total.reveal<std::uint32_t>(ALICE);

    cout << sorted << endl;
    cout << result << endl;
}

int main(int argc, char** argv) {
    int size;
    if (argc == 3) {
        size = 128;
    } else if (argc == 4) {
        size = atoi(argv[3]);
    } else {
        cout << "Usage: " << argv[0] << " party port problem_size" << endl;
        return 1;
    }

    int port, party;
    parse_party_and_port(argv, &party, &port);
    NetIO * io = new NetIO(party==ALICE ? nullptr : "10.0.0.246", port);

    setup_semi_honest(io, party);

    auto start = std::chrono::steady_clock::now();
    aspirin(size);
    auto end = std::chrono::steady_clock::now();

    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cerr << ms.count() << " ms" << endl;
    delete io;
}
