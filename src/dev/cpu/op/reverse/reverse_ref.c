/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2020, OPEN AI LAB
 * Author: qtang@openailab.com
 */

#include "sys_port.h"
#include "module.h"
#include "tengine_errno.h"
#include "tengine_log.h"
#include "tengine_ir.h"
#include "../../cpu_node_ops.h"
#include "tengine_op.h"
#include <math.h>

struct reverse_param
{
    int in_shape[4];    // the dim of the input
    int dim_size;
};

int ref_reverse_fp32(void* input, void* input_axis, void* output, const struct reverse_param* param, int num_thread)
{
    float* out_ptr = output;
    float* in_ptr = input;
    int* axis_ptr = input_axis;
    int axis = axis_ptr[0];

    int in_w = param->in_shape[3];
    int in_hw = param->in_shape[2] * in_w;
    int in_chw = param->in_shape[1] * in_hw;

    if (param->dim_size == 4)
    {
        if (axis == 0 || axis == -4)
        {
            for (int i = 0; i < param->in_shape[0]; i++)
            {
                for (int j = 0; j < param->in_shape[1]; j++)
                {
                    for (int y = 0; y < param->in_shape[2]; y++)
                    {
                        for (int x = 0; x < param->in_shape[3]; x++)
                        {
                            out_ptr[i * in_chw + j * in_hw + y * in_w + x] =
                                in_ptr[(param->in_shape[0] - 1 - i) * in_chw + j * in_hw + y * in_w + x];
                        }
                    }
                }
            }
        }

        else if (axis == 1 || axis == -3)
        {
            for (int i = 0; i < param->in_shape[0]; i++)
            {
                for (int j = 0; j < param->in_shape[1]; j++)
                {
                    for (int y = 0; y < param->in_shape[2]; y++)
                    {
                        for (int x = 0; x < param->in_shape[3]; x++)
                        {
                            out_ptr[i * in_chw + j * in_hw + y * in_w + x] =
                                in_ptr[i * in_chw + (param->in_shape[1] - 1 - j) * in_hw + y * in_w + x];
                        }
                    }
                }
            }
        }

        else if (axis == 2 || axis == -2)
        {
            for (int i = 0; i < param->in_shape[0]; i++)
            {
                for (int j = 0; j < param->in_shape[1]; j++)
                {
                    for (int y = 0; y < param->in_shape[2]; y++)
                    {
                        for (int x = 0; x < param->in_shape[3]; x++)
                        {
                            out_ptr[i * in_chw + j * in_hw + y * in_w + x] =
                                in_ptr[i * in_chw + j * in_hw + (param->in_shape[2] - 1 - y) * in_w + x];
                        }
                    }
                }
            }
        }

        else if (axis == 3 || axis == -1)
        {
            for (int i = 0; i < param->in_shape[0]; i++)
            {
                for (int j = 0; j < param->in_shape[1]; j++)
                {
                    for (int y = 0; y < param->in_shape[2]; y++)
                    {
                        for (int x = 0; x < param->in_shape[3]; x++)
                        {
                            out_ptr[i * in_chw + j * in_hw + y * in_w + x] =
                                in_ptr[i * in_chw + j * in_hw + y * in_w + (param->in_shape[3] - 1 - x)];
                        }
                    }
                }
            }
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

static int init_node(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    return 0;
}

static int release_node(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    return 0;
}

static int prerun(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    return 0;
}

static int run(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    struct ir_node* ir_node = exec_node->ir_node;
    struct ir_graph* ir_graph = ir_node->graph;
    struct ir_tensor* input_tensor;
    struct ir_tensor* axis_tensor;
    struct ir_tensor* output_tensor;
    int layout = ir_graph->graph_layout;

    input_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[0]);
    axis_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[1]);
    output_tensor = get_ir_graph_tensor(ir_graph, ir_node->output_tensors[0]);

    struct reverse_param reverse_param;
    reverse_param.dim_size = input_tensor->dim_num;

    for (int i = 0; i < reverse_param.dim_size; i++)
    {
        reverse_param.in_shape[i] = input_tensor->dims[i];
    }

    int ret = ref_reverse_fp32(input_tensor->data, axis_tensor->data, output_tensor->data, &reverse_param,
                               exec_graph->num_thread);
    if (ret != 0)
        return -1;

    return 0;
}

static int score(struct node_ops* node_ops, struct exec_graph* exec_graph, struct ir_node* exec_node)
{
    return OPS_SCORE_CANDO;
}

static struct node_ops hcl_node_ops = {.prerun = prerun,
                                       .run = run,
                                       .reshape = NULL,
                                       .postrun = NULL,
                                       .init_node = init_node,
                                       .release_node = release_node,
                                       .score = score};

static int reg_reverse_hcl_ops(void* arg)
{
    return register_builtin_node_ops(OP_REVERSE, &hcl_node_ops);
}

static int unreg_reverse_hcl_ops(void* arg)
{
    return unregister_builtin_node_ops(OP_REVERSE, &hcl_node_ops);
}

AUTO_REGISTER_OPS(reg_reverse_hcl_ops);
AUTO_UNREGISTER_OPS(unreg_reverse_hcl_ops);
