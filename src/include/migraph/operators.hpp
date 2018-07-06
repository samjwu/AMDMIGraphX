#ifndef MIGRAPH_GUARD_OPERATORS_HPP
#define MIGRAPH_GUARD_OPERATORS_HPP

#include <array>
#include <migraph/operation.hpp>
#include <migraph/stringutils.hpp>
#include <migraph/streamutils.hpp>
#include <cmath>

namespace migraph {

struct check_shapes
{
    const std::vector<shape>* shapes;
    const std::string name;

    check_shapes(const std::vector<shape>& s) : shapes(&s) {}

    template <class Op>
    check_shapes(const std::vector<shape>& s, const Op& op) : shapes(&s), name(op.name())
    {
    }

    std::string prefix() const
    {
        if(name.empty())
            return "";
        else
            return name + ": ";
    }

    const check_shapes& has(std::size_t n) const
    {
        assert(shapes != nullptr);
        if(shapes->size() != n)
            MIGRAPH_THROW(prefix() + "Wrong number of arguments: expected " + std::to_string(n) +
                          " but given " + std::to_string(shapes->size()));
        return *this;
    }

    const check_shapes& only_dims(std::size_t n) const
    {
        assert(shapes != nullptr);
        if(!shapes->empty())
        {
            if(shapes->front().lens().size() != n)
                MIGRAPH_THROW(prefix() + "Only " + std::to_string(n) + "d supported");
        }
        return *this;
    }

    const check_shapes& same_shape() const
    {
        if(!this->same([](const shape& s) { return s; }))
            MIGRAPH_THROW(prefix() + "Shapes do not match");
        return *this;
    }

    const check_shapes& same_type() const
    {
        if(!this->same([](const shape& s) { return s.type(); }))
            MIGRAPH_THROW(prefix() + "Types do not match");
        return *this;
    }

    const check_shapes& same_dims() const
    {
        if(!this->same([](const shape& s) { return s.lens(); }))
            MIGRAPH_THROW(prefix() + "Dimensions do not match");
        return *this;
    }

    const check_shapes& same_ndims() const
    {
        if(!this->same([](const shape& s) { return s.lens().size(); }))
            MIGRAPH_THROW(prefix() + "Dimensions do not match");
        return *this;
    }

    template <class F>
    bool same(F f) const
    {
        assert(shapes != nullptr);
        if(shapes->empty())
            return true;
        auto&& key = f(shapes->front());
        return this->all_of([&](const shape& s) { return f(s) == key; });
    }

    template <class Predicate>
    bool all_of(Predicate p) const
    {
        assert(shapes != nullptr);
        return std::all_of(shapes->begin(), shapes->end(), p);
    }
};

struct not_computable
{
    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }
};

struct convolution
{
    std::array<std::size_t, 2> padding  = {{0, 0}};
    std::array<std::size_t, 2> stride   = {{1, 1}};
    std::array<std::size_t, 2> dilation = {{1, 1}};
    enum padding_mode_t
    {
        default_, // NOLINT
        same,
        valid
    };
    padding_mode_t padding_mode = default_;
    std::string name() const { return "convolution"; }
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs, *this}.has(2).same_type().same_ndims().only_dims(4);

        const shape& input   = inputs.at(0);
        const shape& weights = inputs.at(1);
        auto t               = input.type();
        if(padding_mode == default_)
        {
            return {t,
                    {
                        input.lens()[0],
                        weights.lens()[0],
                        std::size_t(std::max<std::ptrdiff_t>(
                            1,
                            (input.lens()[2] - (1 + dilation[0] * (weights.lens()[2] - 1)) +
                             2 * padding[0]) /
                                    stride[0] +
                                1)),
                        std::size_t(std::max<std::ptrdiff_t>(
                            1,
                            (input.lens()[3] - (1 + dilation[1] * (weights.lens()[3] - 1)) +
                             2 * padding[1]) /
                                    stride[1] +
                                1)),
                    }};
        }
        else if(padding_mode == same)
        {
            return {t,
                    {input.lens()[0],
                     weights.lens()[0],
                     static_cast<std::size_t>(
                         std::ceil(static_cast<double>(input.lens()[2]) / stride[0])),
                     static_cast<std::size_t>(
                         std::ceil(static_cast<double>(input.lens()[3]) / stride[1]))}};
        }
        else if(padding_mode == valid)
        {
            return {
                t,
                {input.lens()[0],
                 weights.lens()[0],
                 static_cast<std::size_t>(std::ceil(
                     static_cast<double>(input.lens()[2] - weights.lens()[2] + 1) / stride[0])),
                 static_cast<std::size_t>(std::ceil(
                     static_cast<double>(input.lens()[3] - weights.lens()[3] + 1) / stride[1]))}};
        }
        else
        {
            MIGRAPH_THROW("Invalid padding mode");
        }
    }

    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }

    friend std::ostream& operator<<(std::ostream& os, const convolution& op)
    {
        os << op.name() << "[";
        os << "padding={" << stream_range(op.padding) << "}, ";
        os << "stride={" << stream_range(op.stride) << "}, ";
        os << "dilation={" << stream_range(op.dilation) << "}";
        os << "]";
        return os;
    }
};

struct pooling
{
    std::string mode;
    std::array<std::size_t, 2> padding = {{0, 0}};
    std::array<std::size_t, 2> stride  = {{1, 1}};
    std::array<std::size_t, 2> lengths = {{1, 1}};
    std::string name() const { return "pooling"; }
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs, *this}.has(1).only_dims(4);

        const shape& input = inputs.at(0);
        auto t             = input.type();

        assert(lengths[0] < (input.lens()[2] + 2 * padding[0]));
        assert(lengths[1] < (input.lens()[3] + 2 * padding[1]));

        return {t,
                {
                    input.lens()[0],
                    input.lens()[1],
                    std::size_t(std::max<std::ptrdiff_t>(
                        1,
                        std::ceil((input.lens()[2] + 2 * padding[0] - lengths[0]) /
                                  static_cast<float>(stride[0])) +
                            1)),
                    std::size_t(std::max<std::ptrdiff_t>(
                        1,
                        std::ceil((input.lens()[3] + 2 * padding[1] - lengths[1]) /
                                  static_cast<float>(stride[1])) +
                            1)),
                }};
    }

    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }

    friend std::ostream& operator<<(std::ostream& os, const pooling& op)
    {
        os << op.name() << "[";
        os << "padding={" << stream_range(op.padding) << "}, ";
        os << "stride={" << stream_range(op.stride) << "}, ";
        os << "lengths={" << stream_range(op.lengths) << "}";
        os << "]";
        return os;
    }
};

struct activation
{
    std::string mode;
    std::string name() const { return "activation"; }
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs, *this}.has(1);
        return inputs.front();
    }

    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }
    friend std::ostream& operator<<(std::ostream& os, const activation& op)
    {
        os << op.name() << ":" << op.mode;
        return os;
    }
};

struct transpose
{
    std::vector<int64_t> dims;
    std::string name() const { return "transpose"; }
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs, *this}.has(1);
        auto input         = inputs.at(0);
        auto input_lens    = input.lens();
        auto input_strides = input.strides();
        auto t             = input.type();
        if(dims.size() != input_lens.size())
        {
            MIGRAPH_THROW("Permutation has wrong number of axes");
        }
        std::vector<int64_t> axes(dims.size());
        std::iota(axes.begin(), axes.end(), 0);
        if(!std::is_permutation(axes.begin(), axes.end(), dims.begin()))
        {
            MIGRAPH_THROW("Invalid permutation");
        }
        std::vector<size_t> output_lens(input_lens.size());
        std::vector<size_t> output_strides(input_lens.size());
        for(int i = 0; i < output_lens.size(); i++)
        {
            output_lens[i]    = input_lens[dims[i]];
            output_strides[i] = input_strides[dims[i]];
        }
        return {t, output_lens, output_strides};
    }
    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }
};

struct contiguous
{
    std::string name() const { return "contiguous"; }
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs, *this}.has(1);
        auto lens = inputs.at(0).lens();
        auto t    = inputs.at(0).type();
        if(lens.size() < 2)
        {
            MIGRAPH_THROW("Number of dimensions should exceed 1");
        }
        return {t, lens};
    }
    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }
};

struct reshape
{
    std::vector<int64_t> dims;
    std::string name() const { return "reshape"; }
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs, *this}.has(1);
        auto&& idims = inputs.front().lens();
        std::vector<std::size_t> rdims(dims.begin(), dims.end());
        for(std::size_t i = 0; i < dims.size(); i++)
        {
            if(dims[i] == 0)
                rdims[i] = idims[i];
        }
        if(dims.back() == -1)
        {
            rdims.pop_back();
            std::copy(idims.begin() + rdims.size(), idims.end(), std::back_inserter(rdims));
        }
        shape s{inputs.front().type(), rdims};
        if(s.elements() != inputs.front().elements())
            MIGRAPH_THROW("Wrong number of elements for reshape");
        return s;
    }

    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }

    friend std::ostream& operator<<(std::ostream& os, const reshape& op)
    {
        os << op.name() << "[";
        os << "dims={" << stream_range(op.dims) << "}, ";
        os << "]";
        return os;
    }
};

struct gemm
{
    std::string name() const { return "gemm"; }
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs, *this}.has(2).same_type();
        const shape& a = inputs.at(0);
        const shape& b = inputs.at(1);
        auto t         = a.type();

        if(a.lens()[1] != b.lens()[0])
            MIGRAPH_THROW("Inner dimensions do not match");
        return {t, {a.lens()[0], b.lens()[1]}};
    }

    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }

    friend std::ostream& operator<<(std::ostream& os, const gemm& op)
    {
        os << op.name() << "[";
        os << "]";
        return os;
    }
};

struct unary
{
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs}.has(1);
        return inputs.at(0);
    }
    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }
};

struct identity : unary
{
    std::string name() const { return "identity"; }
};

struct abs : unary
{
    std::string name() const { return "abs"; }
};

struct exp : unary
{
    std::string name() const { return "exp"; }
};

struct sin : unary
{
    std::string name() const { return "sin"; }
};

struct cos : unary
{
    std::string name() const { return "cos"; }
};

struct tan : unary
{
    std::string name() const { return "tan"; }
};

struct asin : unary
{
    std::string name() const { return "asin"; }
};

struct acos : unary
{
    std::string name() const { return "acos"; }
};

struct atan : unary
{
    std::string name() const { return "atan"; }
};

struct softmax : unary
{
    std::string name() const { return "softmax"; }
};

struct tanh : unary
{
    std::string name() const { return "tanh"; }
};

struct sigmoid : unary
{
    std::string name() const { return "sigmoid"; }
};

struct neg : unary
{
    std::string name() const { return "neg"; }
};

struct flatten
{
    std::string name() const { return "flatten"; }
};

struct broadcast
{
    uint64_t axis = 0;
    std::string name() const { return "broadcast"; }
    shape compute_shape(std::vector<shape> inputs) const
    {
        auto t      = inputs.at(0).type();
        auto result = inputs.at(0);
        auto input  = inputs.at(1);

        std::vector<size_t> bcast_strides(result.lens().size(), 0);
        if(std::all_of(
               result.lens().cbegin(), result.lens().cend(), [&](auto x) { return x == 1; }))
        {
            if(axis != 0)
                MIGRAPH_THROW("when broadcasting tensor of size 1, axis should be 0");
            return {t, result.lens(), std::move(bcast_strides)};
        }
        else
        {
            assert(result.lens().size() - axis >= input.lens().size());
            if(!std::equal(input.lens().begin(), input.lens().end(), result.lens().begin() + axis))
                MIGRAPH_THROW("when broadcasting success sizes must match");
            std::copy(input.strides().begin(), input.strides().end(), bcast_strides.begin() + axis);
            return {t, result.lens(), std::move(bcast_strides)};
        }
    }
    argument compute(context&, shape output_shape, std::vector<argument> args) const
    {
        return {output_shape, std::move(args.at(1).data)};
    }
};

struct binary
{
    uint64_t broadcast = 0;
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs}.has(2).same_type().same_dims();
        return inputs.at(0);
    }
    argument compute(context&, shape, std::vector<argument>) const
    {
        MIGRAPH_THROW("not computable");
    }
};

struct add : binary
{
    std::string name() const { return "add"; }
};

struct sub : binary
{
    std::string name() const { return "sub"; }
};

struct mul : binary
{
    std::string name() const { return "mul"; }
};

struct div : binary
{
    std::string name() const { return "div"; }
};

struct outline
{
    shape s;
    std::string name() const { return "outline"; }
    shape compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs, *this}.has(0);
        return s;
    }
    argument compute(context&, shape, std::vector<argument>) const { return {s, nullptr}; }
};

template <class T>
struct check_context
{
    std::string name() const { return "check_context"; }
    shape compute_shape(std::vector<shape>) const { return {}; }
    argument compute(context& ctx, shape, std::vector<argument>) const
    {
        T* x = any_cast<T>(&ctx);
        if(x == nullptr)
            MIGRAPH_THROW(std::string("Unexpected context type: ") + ctx.type_id().name());
        return {};
    }
};

} // namespace migraph

#endif