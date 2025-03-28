/* Implement this class. */

import java.util.List;

public class MyDispatcher extends Dispatcher {
    private int rrCounter = -1;

    public MyDispatcher(SchedulingAlgorithm algorithm, List<Host> hosts) {
        super(algorithm, hosts);
    }

    @Override
    public synchronized void addTask(Task task) {
        // decide on which host I need to send the task to & send it with host's addTask() method
        if (algorithm == SchedulingAlgorithm.ROUND_ROBIN) {
            // System.out.println("---------------------------------- Round Robin ----------------------------------");
            int host = (rrCounter + 1) % hosts.size();
            rrCounter ++;
            
            // System.out.println("Assigning -TASK " + task.getId() + "- to -HOST "  + host + "-");
            hosts.get(host).addTask(task);

        } else if (algorithm == SchedulingAlgorithm.SHORTEST_QUEUE) {
            // System.out.println("--------------------------------- Shortest Queue ---------------------------------");
            int min = hosts.get(0).getQueueSize();
            Host selectedHost = hosts.get(0);

            for (Host host : hosts) {
                int queueSize = host.getQueueSize();
                if (queueSize < min) {
                    selectedHost = host;
                    min = queueSize;
                }
            }

            // System.out.println("Assigning -TASK " + task.getId() + "- to -HOST "  + selectedHost.getId() + "-");
            selectedHost.addTask(task);

        } else if (algorithm == SchedulingAlgorithm.SIZE_INTERVAL_TASK_ASSIGNMENT) {
            // System.out.println("------------------------------------- SITA -------------------------------------");
            if (task.getType() == TaskType.SHORT) {
                hosts.get(0).addTask(task);

            } else if (task.getType() == TaskType.MEDIUM) {
                hosts.get(1).addTask(task);
            
            } else if (task.getType() == TaskType.LONG) {
                hosts.get(2).addTask(task);

            }

        } else if (algorithm == SchedulingAlgorithm.LEAST_WORK_LEFT) {
            // System.out.println("-------------------------------- Least Work Left --------------------------------");
            long min = hosts.get(0).getWorkLeft();
            Host selectedHost = hosts.get(0);

            for (int i = 1; i < hosts.size(); i ++) {
                long workLeft = hosts.get(i).getWorkLeft();
                if (workLeft < min) {
                    selectedHost = hosts.get(i);
                    min = workLeft;
                }
            }

            // System.out.println("Assigning -TASK " + task.getId() + "- to -HOST "  + selectedHost.getId() + "-");
            selectedHost.addTask(task);
        }
    }
}
