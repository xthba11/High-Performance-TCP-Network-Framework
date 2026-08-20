# Test cases

## Implemented

- Incomplete protocol header is retained as an incomplete frame.
- A complete frame round-trips through encode and parse.
- Two concatenated frames parse with the first frame's exact consumed length.
- Invalid magic is rejected.
- Body length is bounded by the protocol maximum.

## Linux integration checks

1. Start `hp_server` and connect with `hp_client`.
2. Send one frame split across multiple `send` calls.
3. Send two frames in one TCP write.
4. Close the client while the server is reading.
5. Send a frame larger than the configured read buffer and verify the server closes it.
6. Run the protocol test and server under ASan.
7. Repeat with Valgrind and inspect open descriptors after shutdown.

The performance report must only contain measurements collected on the target Linux machine.
