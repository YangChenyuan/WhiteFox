@register_graph_pattern(
    CallFunction(
        torch.tanh,
        CallFunction(
            torch.stack,
            getitem_split,
            dim=Ignored(),
        ),
    ),
    pass_dict=merge_getitem_cat_pass,
    extra_check=config_flag("split_cat_fx_passes"),
)
@register_graph_pattern(
    CallFunction(
        torch.tanh,
        CallFunction(
            torch.stack,
            tensors=getitem_split,
            dim=Ignored(),
        ),
    ),
    pass_dict=merge_getitem_cat_pass,
    extra_check=config_flag("split_cat_fx_passes"),
)
@register_graph_pattern(
    CallFunction(
        torch.tanh,
        CallFunction(
            torch.stack,
            getitem_split,
            Ignored(),
        ),
    ),
    pass_dict=merge_getitem_cat_pass,
    extra_check=config_flag("split_cat_fx_passes"),
)