#include <iostream>
#include <string>
using namespace std;

// XOR operation for binary strings
string xor_op(string a, string b) {
    string result = "";
    for (int i = 1; i < b.length(); i++) {
        // If bits are same, XOR is 0; otherwise 1
        if (a[i] == b[i])
            result += "0";
        else
            result += "1";
    }
    return result;
}

// Calculate CRC 
string calc_crc(string msg, string key) {
    int msgLen = msg.length();
    int keyLen = key.length();
    
    // Append zeros to message
    string appended = msg;
    for (int i = 0; i < keyLen - 1; i++)
        appended += "0";
    
    string div = appended.substr(0, keyLen);
    
    // Perform division
    for (int i = keyLen; i <= appended.length(); i++) {
        if (div[0] == '1') {
            // If first bit is 1, XOR with key
            div = xor_op(div, key);
        } else {
            // If first bit is 0, XOR with all zeros
            div = xor_op(div, string(keyLen, '0'));
        }
        
        // Bring down next bit
        if (i < appended.length())
            div += appended[i];
    }
    
    return div;
}

// Flip a bit to simulate transmission error
void flip_bit(string &data, int pos) {
    if (pos >= 0 && pos < data.length()) {
        data[pos] = (data[pos] == '0') ? '1' : '0';  // Flip the bit
    }
}

int main() {
    // Get input data
    string data, poly;
    cout << "Enter binary data: ";
    cin >> data;
    
    cout << "Enter generator polynomial (in binary): ";
    cin >> poly;
    
    // Sender side
    string crc = calc_crc(data, poly);
    string tx_data = data + crc;  // Data to transmit
    
    cout << "\nTransmitted Data (Data + CRC): " << tx_data << endl;
    
    // Error simulation
    char choice;
    cout << "Introduce error in transmission? (y/n): ";
    cin >> choice;
    
    if (choice == 'y' || choice == 'Y') {
        int err_pos;
        cout << "Enter position to flip (0-based index): ";
        cin >> err_pos;
        
        flip_bit(tx_data, err_pos);
        cout << "Data after introducing error: " << tx_data << endl;
    }
    
    // Receiver side - error detection
    string rx_crc = calc_crc(tx_data.substr(0, tx_data.length() - poly.length() + 1), poly);
    
    if (rx_crc == crc) {
        cout << "\nNo error detected at receiver." << endl;
    } else {
        cout << "\nError detected at receiver!" << endl;
    }
    
    return 0;
}
