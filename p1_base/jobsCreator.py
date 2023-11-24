# Run me on the ./p1_base folder
#
def setContent(n):
    return f"CREATE {n} 10 10\nRESERVE {n} [(1,1) (10,10)]\nSHOW {n}\n"

def createJobs():
    for i in range(4,26):
        filename = f"{chr(i+ord('a'))}.jobs"
        with open(f"./jobs/{filename}", 'w') as f:
            f.write(setContent(i+2))
    return 0

if __name__ == "__main__":
    
    createJobs()
    print("Done!")