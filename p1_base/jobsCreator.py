if __name__ == "__main__":
    for i in range(1,1001):
        f = open(f"./jobs/{i}.jobs", 'w')
        f.write(f"CREATE {i} 50 50\nRESERVE {i} [(1,1) (50,50)]\nSHOW {i}\n")
        f.close()