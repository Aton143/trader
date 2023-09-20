
def main():
    associations = [
        [ 0, 42, 11],
        [ 1, 43, -1],
        [ 2, 44, 27],
        [ 3, 14, -1],
        [ 4, -1, -1],
        [ 5, 30, -1],
        [ 6, 17, 45],
        [ 7, 46, -1],
        [ 8, 47, 33],

        [10, 39, -1],
        [13, -1, -1],
        [16, 48, -1],
        [40, -1, -1],
        [41, 28, -1],
        [31, -1, -1],
        [34, 50, -1],
        [49, -1, -1],

        [18, 38, 29],
        [19, 37, -1],
        [20, 36,  9],
        [21, 32, -1],
        [22, -1, -1],
        [23, 12, -1],
        [24, 35, 53],
        [25, 52, -1],
        [26, 15, 51] ]

    i = 0
    print("{")
    for indices in associations:
        mappings = [[[-1, -1, "-1"], [-1, -1, "-1"]] for n in range(6)]

        print(f"// {i}")
        for index in indices:
            if index == -1:
                break

            mapping_index = index // 9
            mod = index % 9
            row = mod // 3
            col = mod % 3

            mappings[mapping_index][0][0] = mapping_index
            mappings[mapping_index][0][1] = row
            mappings[mapping_index][0][2] = "oud"

            mappings[mapping_index][1][0] = mapping_index
            mappings[mapping_index][1][1] = col
            mappings[mapping_index][1][2] = "olr"
            
        print("{ ")

        for mapping in mappings:
            print("{", end="")
            print("{" + str(mapping[0][0]) + ", " + str(mapping[0][1]) + ", " + str(mapping[0][2]) + "},")
            print("{" + str(mapping[1][0]) + ", " + str(mapping[1][1]) + ", " + str(mapping[1][2]) + "}", end="")
            print("},")

            # print("{")
            # for value in mapping[1]:
            #     print(f"{value},")
            # print("}")

        print("},\n")
        i += 1

    print("}")


if __name__ == "__main__":
    main()
