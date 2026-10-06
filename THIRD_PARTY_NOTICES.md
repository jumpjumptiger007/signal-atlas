# Third-Party Notices

## reelyActive iBeacon decoder logic

The bounded iBeacon framing decoder in `main/identify/protocol_decoder.c` is a C adaptation of the v0.1 iBeacon layout documented by `reelyActive/advlib-ble-manufacturers`, including its valid test vector in `tests/test_radio_model.c`.

Repository: <https://github.com/reelyactive/advlib-ble-manufacturers>
Revision: `952b448e12c2a60ea2cf9a4e22b4dd10e5540112`
License: MIT

Copyright (c) 2021-2026 reelyActive

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## reelyActive Eddystone decoder logic

The bounded Eddystone UID/URL/unsecured-TLM field layouts and their valid vectors are adapted from `reelyActive/advlib-ble-services`. EID, encrypted TLM, and Find Hub behavior are not included.

Repository: <https://github.com/reelyActive/advlib-ble-services>
Revision: `1351ff0a63aa8e170cb0c9807423739641d9de6f`
License: MIT

Copyright (c) 2020-2026 reelyActive

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Nordic Bluetooth Numbers Database

Generated Bluetooth Company ID, Service UUID, and Appearance names in `main/identify/registry_data.c` are built from Nordic Semiconductor's Assigned Numbers JSON data at the pinned revision below. The generated tables are transformed data; the upstream JSON inputs and their hashes are recorded in `main/identify/registry_metadata.json`.

Repository: <https://github.com/nordicsemi/bluetooth-numbers-database>
Revision: `d379cc06c9c3681bc1c1cd4d61c246a0b0f80b0e`
License: BSD-3-Clause

Copyright (c) 2019 - 2020, Nordic Semiconductor ASA
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

## IEEE Registration Authority public listings

The firmware retains IEEE MA-L/MA-M/MA-S longest-prefix lookup code and local-generation support, but this repository does not distribute IEEE source CSVs or IEEE-derived tables. The default/public build reports zero IEEE records. Developers may independently obtain the official listings from <https://standards.ieee.org/products-programs/regauth/> and follow the local-only instructions in `README.md`. Generated local IEEE files are ignored and must not be redistributed.

The IEEE Registration Authority FAQ states that an assignment segment cannot be distributed by anyone other than IEEE. No permission to redistribute transformed listing data is claimed here; this project takes the conservative approach of excluding it from source and default/public firmware until permission is confirmed. See <https://standards.ieee.org/faqs/regauth/>.
