#include <chrono>
#include "emp-sh2pc/emp-sh2pc.h"
using namespace emp;
using namespace std;

// struct input {
//     Integer patient_id_concat_timestamp;
//     Bit diagnosis; // or aspirin prescription
// };

void merge_sorted(int party, int input_size_per_party, int key_bits = 32, int value_bits = 96) {
    int input_array_length = input_size_per_party * 2;
    Integer* key = new Integer[input_array_length];
    Integer* value = new Integer[input_array_length];
    for (int i = 0; i != input_array_length; i++) {
        if (i < input_size_per_party) {
            key[i] = Integer(key_bits, i, ALICE);
            value[i] = Integer(value_bits, i, ALICE);
        } else {
            key[i] = Integer(key_bits, input_array_length - i - 1, BOB);
            value[i] = Integer(value_bits, input_array_length - i - 1, BOB);
        }
    }

    // Verify the input first.
    // Bit verifyOrder(true);
    // for (int i = 0; i < input_size_per_party - 1; i++) {
    //     Bit lessThanNext = !(patient_id_concat_timestamp[i] > patient_id_concat_timestamp[i+1]);
    //     verifyOrder = verifyOrder & lessThanNext;
    // }
    // for (int i = input_size_per_party; i < 2 * input_size_per_party - 1; i++) {
    //     Bit greaterThanNext = patient_id_concat_timestamp[i].geq(patient_id_concat_timestamp[i+1]);
    //     verifyOrder = verifyOrder & greaterThanNext;
    // }
    // bool sorted = verifyOrder.reveal<bool>(ALICE);

    // Merge the two arrays, sorted ascending by patient_id_concat_timestamp
    auto start = std::chrono::steady_clock::now();
    bitonic_merge(key, value, 0, input_array_length, true);
    auto end = std::chrono::steady_clock::now();
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << "Compute: " << ms.count() << " ms" << endl;

    // Integer total(result_bits, 0, PUBLIC);
    // Integer one(result_bits, 1, PUBLIC);
    //
    // // Now, for each input, check if it and the next input have the same patient, but the first is a diagnosis and the second isn't.
    // for (int i = 0; i < input_array_length; i++) {
    //     Bit add = inputs[i].diagnosis & !inputs[i+1].diagnosis;
    //     add = add & ((inputs[i].patient_id_concat_timestamp >> timestamp_bits).equal(inputs[i+1].patient_id_concat_timestamp >> timestamp_bits));
    //     Integer next = total + one;
    //     total.select(add, next);
    // }
    //
    // std::uint32_t result = total.reveal<std::uint32_t>(ALICE);
    // delete[] inputs;
    //
    // cout << sorted << endl;
    // cout << result << endl;
    bool passed = true;
    for (int i = 0; i < input_array_length; i++) {
        std::uint32_t key_result = key[i].reveal<std::uint32_t>(ALICE);
        std::uint32_t value_result = value[i].reveal<std::uint32_t>(ALICE);
        if (party == ALICE) {
            if (key_result != static_cast<std::uint32_t>(i >> 1) || value_result != static_cast<std::uint32_t>(i >> 1)) {
                std::cout << "Wrong result at index " << i << " (expected " << (i >> 1) << " but got " << key_result << ", " << value_result << ")" << std::endl;
                passed = false;
            }
        }
    }
    if (passed) {
        std::cout << "PASS" << std::endl;
    }

    delete[] key;
    delete[] value;
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
    NetIO * io = new NetIO(party==ALICE ? nullptr : "127.0.0.1", port);

    setup_semi_honest(io, party);

    auto start = std::chrono::steady_clock::now();
    merge_sorted(party, size);
    auto end = std::chrono::steady_clock::now();

    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << "Total: " << ms.count() << " ms" << endl;
    delete io;
}
