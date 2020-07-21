Dusty's Simulated Annealing Schedule
====================================

This simulated annealing schedule is designed to quickly characterize the search space and maintain a target success ratio (accepted moves.)

It starts at the minimum alpha (``--alpha_min``) to allow it to quickly find the target.

For each alpha, the temperature decays by a factor of alpha after each outer loop iteration.

The temperature before which the success ratio drops below the target (``--anneal_success_target``) is recorded; after hitting the minimum success ratio (``--anneal_success_min``), the temperature resets to a little before recorded temperature, and alpha parameter itself decays according to ``--alpha_decay``.

The effect of this is many fast, but slowing sweeps in temperature, focused where they can make the most effective progress. Unlike fixed and adaptive schedules that monotonically decrease temperature, this allows the global properties of the search space to affect the schedule.

In addition, move_lim (which controls the number of iterations in the inner loop) is scaled with the target success ratio over the current success ratio, which reduces the time to reach the target ratio.

The schedule terminates when the maximum alpha (``--alpha_max``) is reached. Termination is ensured by the narrowing range between the recorded upper temperature and the minimum success ratio, which will eventually cause alpha to reach its maximum.

This algorithm was inspired by Lester Ingber's adaptive simulated annealing algorithm [ASA93]_.

See ``update_state()`` in ``place.cpp`` for the algorithm details.

.. [ASA93] Ingber, Lester. "Adaptive simulated annealing (ASA)." Global optimization C-code, Caltech Alumni Association, Pasadena, CA (1993).
