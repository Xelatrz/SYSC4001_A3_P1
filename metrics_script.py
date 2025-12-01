import re
from collections import defaultdict

# name depends on the file being checked.
log = open("output_files/execution_EPRR_s16.txt").read().strip().splitlines()

arrival = {}
start = {}
terminate = {}
cpu_start_times = defaultdict(list)
cpu_end_times = defaultdict(list)

pattern = r"\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\w+)\s*\|\s*(\w+)\s*\|"

for line in log:
    match = re.search(pattern, line)
    if not match:
        continue

    time = int(match.group(1))
    pid = int(match.group(2))
    old = match.group(3)
    new = match.group(4)

    if old == "NEW" and new == "READY":
        arrival[pid] = time

    if old == "READY" and new == "RUNNING":
        if pid not in start:
            start[pid] = time
        cpu_start_times[pid].append(time)

    if old == "RUNNING":
        cpu_end_times[pid].append(time)

    if new == "TERMINATED":
        terminate[pid] = time
    
wait_times = []
turnaround_times = []
response_times = []

# name of output file
with open("metrics_EPRR_s16.txt", "w") as f:
    f.write("=== RESULTS ===\n\n")
    for pid in sorted(arrival):
        a = arrival[pid]
        s = start.get(pid, a)
        t = terminate.get(pid, s)

        cpu_time = sum(end - start for start, end in zip(cpu_start_times[pid], cpu_end_times[pid]))

        turnaround = t - a
        response = s - a
        wait = turnaround - cpu_time

        wait_times.append(wait)
        turnaround_times.append(turnaround)
        response_times.append(response)

        f.write(f"PID {pid}: arrival={a}, start={s}, end={t}\n")
        f.write(f"   turnaround={turnaround}, wait={wait}, response={response}, cpu_time={cpu_time}\n")

        makespan = max(terminate.values()) if terminate else 0
        throughput = len(terminate) / makespan if makespan > 0 else 0
        
    f.write("\n=== SUMMARY ===\n")
    f.write(f"Throughput: {throughput:.4f} process per time unit\n")
    if wait_times:
        f.write(f"Average Wait Time: {sum(wait_times)/len(wait_times):.2f}\n")
        f.write(f"Average Turnaround Time: {sum(turnaround_times)/len(turnaround_times):.2f}\n")
        f.write(f"Average Response Time: {sum(response_times)/len(response_times):.2f}\n")
    else:
        f.write("No processes found in log.\n")