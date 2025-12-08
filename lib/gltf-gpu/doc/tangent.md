# Tangent Calculation

### Input
Denote 3 input points as 
$$
p_1, p_2, p_3
$$
with texcoords
$$
t_1, t_2, t_3
$$

Assuming that we are calculating tangent (and bitangent) for point $p_1$

### Vectors

Position vectors in the world space will be
$$
\Delta p_1 = (p_2 - p_1)\\
\Delta p_2 = (p_3 - p_1)
$$
and vectors in the texcoord space will be
$$
\Delta t_1 = (t_2 - t_1) \\
\Delta t_2 = (t_3 - t_1)
$$

### Equation

If the tangent and bitangent are denoted as $T$ and $B$, then
$$
\left[T, B\right] \cdot \Delta t_1 = \Delta p_1 \\
\left[T, B\right] \cdot \Delta t_2 = \Delta p_2
$$
which is
$$
\left[T, B\right] \cdot \left[\Delta t_1, \Delta t_2\right] = \left[\Delta p_1, \Delta p_2\right]
$$
thus
$$
\left[T, B\right] = \left[\Delta p_1, \Delta p_2\right] \cdot \left[\Delta t_1, \Delta t_2\right]^{-1}
$$