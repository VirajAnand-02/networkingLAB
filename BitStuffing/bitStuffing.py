def bit_stuff(data):
    stuffed = ""
    count = 0
    for bit in data:
        if bit == "1":
            count += 1
            stuffed += bit
            if count == 5:
                stuffed += "0"  # Stuff a 0 after five consecutive 1's
                count = 0
        else:
            stuffed += bit
            count = 0
    return stuffed


def bit_destuff(stuffed_data):
    destuffed = ""
    count = 0
    i = 0
    while i < len(stuffed_data):
        bit = stuffed_data[i]
        if bit == "1":
            count += 1
            destuffed += bit
            if count == 5:
                # Skip the stuffed 0
                i += 1
                count = 0
        else:
            destuffed += bit
            count = 0
        i += 1
    return destuffed


def main():
    data = input("Enter the binary data: ").strip()
    flag = input("Enter the flag pattern: ").strip()

    stuffed_data = bit_stuff(data)

    transmitted = flag + stuffed_data + flag
    print("\nTransmitted Message:")
    print(transmitted)

    if transmitted.startswith(flag) and transmitted.endswith(flag):
        extracted_data = transmitted[len(flag) : -len(flag)]
    else:
        print("Warning: Flag pattern not found at start and end!")
        extracted_data = transmitted

    original_data = bit_destuff(extracted_data)
    print("\nReceived Data, De-stuffing:")
    print(original_data)


if __name__ == "__main__":
    main()
